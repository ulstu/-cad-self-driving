from AbstractWorker import AbstractWorker
import torch
import pathlib
import os
import numpy as np
import traceback
from PIL import Image
from ament_index_python.packages import get_package_share_directory
from fastseg import MobileV3Large
from fastseg.image import colorize, blend
from webots_ros2_suv.lib.timeit import timeit
from webots_ros2_suv.lib.lane_line_model import LaneLineModel
from webots_ros2_suv.lib.lane_line_model_utils import get_label_names, draw_lines, draw_segmentation, LaneLine, LaneMask, default_palette
from webots_ros2_suv.lib.map_builder import MapBuilder
from webots_ros2_suv.lib.LinesAnalizator import LinesAnalizator
import cv2
import yaml

PACKAGE_NAME = "webots_ros2_suv"
local_model_path = "resource/lane_line_model/LLD-level-5-v2.pt"
local_model_config_path = "resource/lane_line_model/config.yaml"

class LaneLineDetectionWorker(AbstractWorker):
    def __init__(self, *args, **kwargs) -> None:
        super().__init__(*args, **kwargs)

        package_dir = get_package_share_directory(PACKAGE_NAME)
        model_path = os.path.join(package_dir, local_model_path)
        config_path = os.path.join(package_dir, local_model_config_path)

        self.labels = get_label_names(config_path)
        self.lane_line_model = LaneLineModel(model_path, use_curve_line=True)
        self.lines_analizator = LinesAnalizator()
        

    def on_event(self, event, scene=None):
        return None


    def on_data(self, world_model):
        try:
            if world_model:
                img = np.array(Image.fromarray(world_model.rgb_image))
                line_batches, mask_batches, results = self.lane_line_model.predict([img])
                
                world_model.img_front_objects_lines = self.lane_line_model.generate_prediction_plots([world_model.img_front_objects], self.labels, mask_batches, line_batches)[0]
                world_model.ipm_colorized_lines = np.copy(world_model.ipm_colorized)

                count_roads, car_line_id = self.lines_analizator.analize_roads_with_accamulator(world_model.img_front_objects_lines, 
                                                                                                line_batches, 
                                                                                                0.45, 1.0, 
                                                                                                world_model.img_front_objects_lines.shape)
                
                self.lines_analizator.draw_labels(world_model.img_front_objects_lines, 
                                                  label_names=[f"Count of lanes: {count_roads}", f"Car on lane: {car_line_id}"],
                                                  colors=[(27, 198, 250), (25, 247, 184)])

                lines_bev = []
                masks_bev = []

                line_batches_bev = []
                mask_batches_bev = []
                
                
                for line in line_batches[0]:
                    points_bev = []
                    for point in line.points:
                        x, y = world_model.map_builder.calc_bev_point([point[0], point[1] - world_model.map_builder.get_horizont_line()])
                        points_bev.append([x, y])
                    
                    points_bev = np.array(points_bev, dtype=np.int32)
                    
                    lines_bev.append(LaneLine(points_bev, line.label, line.points_n, line.elapsed_time, line.mask_count_points))
                
                line_batches_bev = [lines_bev]
                
                for mask in mask_batches[0]:
                    points_bev = []
                    for point in mask.points:
                        x, y = world_model.map_builder.calc_bev_point([point[0], point[1] - world_model.map_builder.get_horizont_line()])
                        points_bev.append([x, y])
                    
                    points_bev = np.array(points_bev, dtype=np.int32)

                    masks_bev.append(LaneMask(points_bev, mask.points_n, mask.label, mask.orig_shape))
                
                mask_batches_bev = [masks_bev]

                world_model.lane_contours = mask_batches[0]
                world_model.lane_lines = line_batches[0]
                
                world_model.lane_contours = mask_batches_bev[0]
                world_model.lane_lines = line_batches_bev[0]

                draw_lines([world_model.ipm_colorized_lines], line_batches_bev, palette=[(0, 0, 255)], thickness=4)

                # count_points = [50, 50]
                # obj_image_size = np.array(world_model.img_front_objects_lines.shape[0:2])[::-1]
                # for x in range(count_points[0]):
                #     for y in range(count_points[1]):
                #         point_x = obj_image_size[0] / count_points[0] * x
                #         point_y = obj_image_size[1] / count_points[1] * y

                #         colorR = np.array([255, 0, 0])
                #         colorB = np.array([0, 0, 255])
                #         colorRB = (colorR * (x / count_points[0]) + colorB * (y / count_points[1]))
                #         colorRB = tuple(colorRB.astype(int).tolist())

                #         colorB = np.array([0, 0, 255])
                #         colorG = np.array([0, 255, 0])
                #         colorBG = (colorB * (x / count_points[0]) + colorG * (y / count_points[1]))
                #         colorBG = tuple(colorBG.astype(int).tolist())

                #         point_x_bev, point_y_bev = world_model.map_builder.calc_bev_point((point_x, point_y))

                #         #cv2.circle(world_model.img_front_objects_lines, (int(point_x), int(point_y)), 3, colorRB, -1)
                #         cv2.circle(world_model.img_front_objects_lines, (int(point_x_bev), int(point_y_bev)), 3, colorRB, -1)
                    
                # cv2.circle(world_model.img_front_objects_lines, (img.shape[0], int(img.shape[1] / 2)), 5, (255, 0, 0), -1)

        except  Exception as err:
            super().error(''.join(traceback.TracebackException.from_exception(err).format()))
        
        return world_model