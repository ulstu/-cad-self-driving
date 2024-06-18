from AbstractWorker import AbstractWorker
import cv2
import os
import yaml
import pathlib
import numpy as np
import traceback
import math
from fastseg.image import colorize, blend
from ament_index_python.packages import get_package_share_directory
from filterpy.kalman import KalmanFilter
from webots_ros2_suv.lib.timeit import timeit
from webots_ros2_suv.lib.rrt_star_reeds_shepp import RRTStarReedsShepp
from webots_ros2_suv.lib.rrt_star_direct import rrt_star, RRTTreeNode, visualize_path
from webots_ros2_suv.lib.rrt_star import RRTStar
import matplotlib.pyplot as plt
from webots_ros2_suv.lib.a_star import astar, kalman_filter_path, smooth_path_with_dubins, bezier_curve
from threading import Thread
from geopy.distance import geodesic

class PathPlanningWorker(AbstractWorker):

    def __init__(self, *args, **kwargs) -> None:
        super().__init__( *args, **kwargs)
        package_dir = get_package_share_directory('webots_ros2_suv')
        config_path = os.path.join(package_dir,
                                    pathlib.Path(os.path.join(package_dir, 'config', 'global_coords.yaml')))
        self.turning_radius = 0.1
        self.process_noise_scale = 0.05 
        self.robot_radius=4
        self.step_size=10
        self.sample_size=5

        with open(config_path) as file:
            config = yaml.full_load(file)
            self.turning_radius = config['dubins_turning_radius']
            self.process_noise_scale = config['kalman_path_planning_noise_scale']
            self.robot_radius = config['robot_radius']
            self.step_size = config['a_star_step_size']
            self.sample_size = config['dubins_sample_size']
            self.min_path_points_dist = config['min_path_points_dist']


    def plan_a_star(self, world_model):

        world_model.path = astar(world_model.pov_point, 
                                 world_model.goal_point, 
                                 world_model.ipm_image, 
                                 self.robot_radius, 
                                 self.step_size,
                                 super().log)
        if not world_model.path:
            super().log('A* path not found')
        if world_model.path:
            world_model.path = kalman_filter_path(world_model.path, self.process_noise_scale)
            world_model.path = self.filter_coordinates(world_model.path, self.min_path_points_dist, 10)
            # world_model.path = smooth_path_with_dubins(world_model.path, self.turning_radius, world_model.ipm_image, self.sample_size, super().log)
            world_model.path = bezier_curve(world_model.path, 20)

    def calculate_angle(self, p1, p2, p3):
        """Calculate the angle between the line segments (p1p2) and (p2p3)."""
        v1 = (p1[0] - p2[0], p1[1] - p2[1])
        v2 = (p3[0] - p2[0], p3[1] - p2[1])
        angle = math.atan2(v2[1], v2[0]) - math.atan2(v1[1], v1[0])
        return abs(angle)

    def filter_coordinates(self, coordinates, min_path_points_dist, angle_threshold_degrees):
        if not coordinates:
            return []

        angle_threshold = math.radians(angle_threshold_degrees)
        filtered_coords = [coordinates[0]]

        for i in range(1, len(coordinates)):
            if math.sqrt((filtered_coords[-1][0] - coordinates[i][0]) ** 2 + (filtered_coords[-1][1] - coordinates[i][1]) ** 2) >= min_path_points_dist:
                if len(filtered_coords) < 2 or self.calculate_angle(filtered_coords[-2], filtered_coords[-1], coordinates[i]) >= angle_threshold:
                    filtered_coords.append(coordinates[i])

        return filtered_coords

    def plan_path(self, world_model):
        if not world_model.goal_point:
            return world_model

        world_model.path = None
        self.plan_a_star(world_model)
        if world_model.path:
            super().log(f'PATH len: {len(world_model.path)}')
        # world_model.path = self.kalman_filter_path(world_model.path)
        return world_model

    def on_event(self, event, scene=None):
        print("Emergency State")
        return None

    #@timeit
    def on_data(self, world_model):
        try:
            # super().log(f"PathPlanningWorker {str(world_model)}")
            # thread = Thread(target = self.plan_path, args = (world_model,))
            # thread.start()
            world_model = self.plan_path(world_model)

        except  Exception as err:
            super().error(''.join(traceback.TracebackException.from_exception(err).format()))

        return world_model
