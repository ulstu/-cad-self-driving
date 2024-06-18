extern Mailbox_Handle mbxHandleRFtx; //����� ������� ���������������� ������

///////////////////////////////////////////

void Preset_protocol(void)
{
 uint8_t i1;

 Subscriber.Start=START_BYTE;
 Subscriber.Number_Module=0;
 Subscriber.Role=ROLE_DEVICES;
 Subscriber.TTL=1;
 Subscriber.Repeat=REPEAT_NU;
 Subscriber.Sigma_send=SIGMA_SEND_NU;
 Statistics.Stage=STAGE_IDLE;
 Statistics.Level_now=0;
 Statistics.Number_Queue=0;
 Statistics.Router_Down=0;
 Statistics.Router_Down_Counter_Trust=0;

 for(i1=0; i1<NUMBER_OF_DEVICES; i1++)
 {
  Statistics.Status_devices_in_level_1[i1]=0;
  Statistics.List_devices_in_level_1[i1]=0;
 }

 for(i1=0; i1<MAX_SIZE_HANDOVER_LIST; i1++)
 {
  Handover[i1].Router_Down=0;
  Handover[i1].Counter=0;
 }

 indxPctBuf=0;
 indx_last_packet=0;
 NumberPacket=0;
 time_now=0;
 CounterTrust=0;

 if(Subscriber.Number_Module==1)
 {
  //RF ������. ������ ������ ��������� �������������
  Statistics.Stage=STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS;
  Statistics.Router_Down=1;
 }
}

void Package_processing(uint8_t* Received_packet, uint8_t len, int8_t last_rssi)
{
 //������ ����� ����
 uint8_t prb, indxCounter0, indxLevel;
 pHPacket pPacketReceiv;
 pHPacket pBuf;

 PacketReceiv.len=len;
 memcpy(PacketReceiv.buf, Received_packet, PacketReceiv.len);
 pPacketReceiv=(pHPacket)(PacketReceiv.buf);

 if((pPacketReceiv->Number_Source!=0) && (pPacketReceiv->Number_Destination!=0) && (Subscriber.Number_Module))
 {
  prb=0;
  if(Subscriber.Number_Module!=1)
  {
   if(pPacketReceiv->Type_Comand==TYPE_COMAND_ARCHIVE)
   {
    while(prb<SIZE_PCTBUF)
    {
     pBuf=(pHPacket)&PacketBuf[prb].buf;
     if((pPacketReceiv->Number_Source==pBuf->Number_Source) && (pPacketReceiv->N_Paketa==pBuf->N_Paketa))
     {
      switch (pPacketReceiv->Type_Comand)
      {
       case TYPE_COMAND_ARCHIVE:
          if(pBuf->Type_Comand==TYPE_COMAND_ARCHIVE)
          {
           if(PacketBuf[prb].repeat!=Subscriber.Repeat)
           {
            PacketBuf[prb].repeat=0;
           }
           prb=SIZE_PCTBUF;//����� ����� ��� ��������
          }
       break;
      }
     }
     prb++;
    }
   }
  }

  if(prb!=(SIZE_PCTBUF+1))
  {
   //������ ����� �����
   if(pPacketReceiv->TTL!=0) pPacketReceiv->TTL--;

   switch(pPacketReceiv->Type_Comand)
   {
    //�������� ������� ������ ���������
    case TYPE_COMAND_SURVEY_DEVICES_IN_LEVEL_PLUS:
      if(Subscriber.Number_Module!=1)
      {
       if((pPacketReceiv->Number_Destination==Subscriber.Number_Module) && (pPacketReceiv->Number_Source==Statistics.Router_Down))
       {
        Statistics.Stage=STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS;
        Statistics.Router_Down_Counter_Trust=LIMIT_HANDOVER_TRUST;
       }
       else
       {
        //������ ���������� ��� ��� �� �����
        if(pPacketReceiv->Number_Destination==BROADCAST_ADDRESS)
        {
         if((Statistics.Router_Down==0) || ((Statistics.Router_Down==pPacketReceiv->Number_Source) && (Statistics.Router_Down_Counter_Trust==0)))
         {
          //���� ��� �� ����������
          Statistics.Stage=STAGE_IDLE;
          Statistics.Level_now=pPacketReceiv->Level+1;
          Statistics.Router_Down=pPacketReceiv->Number_Source;

          //��������� ��������� � ������ ������
          pBuf=(pHPacket)&PacketBuf[INDX_PCT_BUF_0].buf;
          pBuf->Start=START_BYTE;
          pBuf->Number_Source=Subscriber.Number_Module;
          pBuf->Number_Destination=pPacketReceiv->Number_Source;
          pBuf->Level=Statistics.Level_now;
          pBuf->N_Paketa=NumberPacket; NumberPacket++;
          pBuf->Status=0;
          pBuf->TTL=1;
          pBuf->Type_Comand=TYPE_COMAND_I_AM_NEW_DEVICES_IN_LEVEL_PLUS;

          PacketBuf[INDX_PCT_BUF_0].repeat=1;
          PacketBuf[INDX_PCT_BUF_0].len=sizeof(HPacket)+1;
          PacketBuf[INDX_PCT_BUF_0].buf[(PacketBuf[INDX_PCT_BUF_0].len-1)]=STOP_BYTE;
          PacketBuf[INDX_PCT_BUF_0].time_delta_for_send=(Subscriber.Sigma_send & rand());
          PacketBuf[INDX_PCT_BUF_0].time_tmp=0;
         }

         indxCounter0=MAX_SIZE_HANDOVER_LIST;
         indxLevel=MAX_SIZE_HANDOVER_LIST;
         prb=0;
         while(prb<MAX_SIZE_HANDOVER_LIST)
         {
          if(Handover[prb].Counter==0) indxCounter0=prb;//��������� ������
          else
          {
           if(Handover[prb].Level>pPacketReceiv->Level) indxLevel=prb;
          }
          if(Handover[prb].Router_Down==pPacketReceiv->Number_Source)
          {
           //����� ������ ����� ������ ���� ��� �����. ��� ������ ������� � indxCounter0
           indxCounter0=prb;
           prb=MAX_SIZE_HANDOVER_LIST;
          }
          prb++;
         }
         prb=0;
         if(indxCounter0<MAX_SIZE_HANDOVER_LIST)
         {
          prb=indxCounter0;
         }
         else
         {
          if(indxLevel<MAX_SIZE_HANDOVER_LIST) prb=indxLevel;
         }
         //��������� ��� ��������� ������� handover
         if(Handover[prb].Counter<LIMIT_HANDOVER_TRUST) Handover[prb].Counter++;
         Handover[prb].Router_Down=pPacketReceiv->Number_Source;
         Handover[prb].Level=pPacketReceiv->Level;
        }
       }
      }

      if(Statistics.Stage==STAGE_RECEIVING_ANSWER_SURVEY_DEVICE_IN_LEVEL_PLUS)
      {
       //��� ��������� ������� �������, ���� ���� ������� �������� ���������� �� ������� ������ �� ����
       if(Statistics.Number_Queue<NUMBER_OF_DEVICES)
       {
        Statistics.Status_devices_in_level_1[Statistics.Number_Queue]=1;
        prb=Statistics.Number_Queue;
       }
       else prb=0;
       if(pPacketReceiv->Number_Source==Statistics.List_devices_in_level_1[prb])
       {
        Statistics.time_Stage=0;//������ ������� ��� Statistics.Stage
        Statistics.time_Stage_limit=SIGMA_WAIT_2s;
        if(Subscriber.Number_Module==1) Statistics.time_Stage_limit=4000;
        }
       }
    break;

    //������� ����� �� ����� ���������
    case TYPE_COMAND_I_AM_NEW_DEVICES_IN_LEVEL_PLUS:
       if((pPacketReceiv->Number_Destination==Subscriber.Number_Module) && ((Statistics.Level_now+1)==pPacketReceiv->Level))
       {
        //������� �� �������� ����� TYPE_COMAND_I_AM_NEW_DEVICES_IN_LEVEL_PLUS
        prb=0;
        while(prb<NUMBER_OF_DEVICES)
        {
         if(Statistics.List_devices_in_level_1[prb]==pPacketReceiv->Number_Source)
         {
          Statistics.Status_devices_in_level_1[prb]=1;
          prb=NUMBER_OF_DEVICES;
         }
         prb++;
        }
        if(prb==NUMBER_OF_DEVICES)
        {
         //��� ����� ����������, �������� � ������ ���
         prb=0;
         while(prb<NUMBER_OF_DEVICES)
         {
          if(Statistics.Status_devices_in_level_1[prb]==0)
          {
           Statistics.List_devices_in_level_1[prb]=pPacketReceiv->Number_Source;
           Statistics.Status_devices_in_level_1[prb]=1;
           prb=NUMBER_OF_DEVICES;
          }
          prb++;
         }
        }
        Statistics.Number_Queue=0;
       }
    break;

    //�������� ������������� ������ �� ����������
    case TYPE_COMAND_ARCHIVE:
       if(pPacketReceiv->Number_Destination==Subscriber.Number_Module)
       {
        //���������, ��������� �� ��� �����
        if((pPacketReceiv->Status&TYPE_PACKET_NULL)==TYPE_PACKET_NULL)
        {
         //��� ��������� ����� ������
         if(Statistics.Number_Queue<NUMBER_OF_DEVICES)
         {
          Statistics.Status_devices_in_level_1[Statistics.Number_Queue]=1;
         }
         Statistics.Number_Queue++;//�������� ��� ������ ������ ����������
         Statistics.Stage=STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS;
        }
        pPacketReceiv->Status&=~TYPE_PACKET_NULL;

        //���� ��������� ������
        prb=0;
        while(prb<(SIZE_PCTBUF-1))
        {
         if(PacketBuf[prb].repeat==0)
         {
          indxPctBuf=prb;//��������� ������, ����� ��������� �� �����������
          prb=SIZE_PCTBUF;
         }
         prb++;
        }

        //�������� � ����� ������� ��� �������������
        pPacketReceiv->Number_Destination=Statistics.Router_Down;
        pPacketReceiv->TTL=1;
        pBuf=(pHPacket)&PacketBuf[indxPctBuf].buf;
        memcpy(PacketBuf[indxPctBuf].buf, PacketReceiv.buf, PacketReceiv.len);//��������� ����� � ������ � ��������� ��� ��������
        PacketBuf[indxPctBuf].repeat=1;
        PacketBuf[indxPctBuf].len=PacketReceiv.len;
        PacketBuf[indxPctBuf].time_delta_for_send=0;
        PacketBuf[indxPctBuf].time_tmp=0;

        if(Subscriber.Number_Module==1) PacketBuf[indxPctBuf].repeat=1;
        else PacketBuf[indxPctBuf].repeat=Subscriber.Repeat;

        indxPctBuf++;
        if(indxPctBuf>=(SIZE_PCTBUF-1)) indxPctBuf=0;

       }
    break;
   }
  }
 }
}

void Procedure_sending_packages(uint8_t** pBuf_sending_packages, uint8_t* pLen_sending_packages)
{
 //�������� ������
 pHPacket pBuf;
 pHPacket pBuf_from_UART;

 uint8_t i1, i2;

 time_now++;
 Statistics.time_Stage++;
 CounterTrust++;

 switch (Statistics.Stage)
 {
  //��������� ������ ��� ������ ����� ���������
  case STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS:
      pBuf=(pHPacket)&PacketBuf[INDX_PCT_BUF_0].buf;

      pBuf->Start=START_BYTE;
      pBuf->Type_Comand=TYPE_COMAND_SURVEY_DEVICES_IN_LEVEL_PLUS;
      pBuf->Number_Source=Subscriber.Number_Module;
      i1=Statistics.Number_Queue;
      i2=NUMBER_OF_DEVICES;
      while(i1<NUMBER_OF_DEVICES)
      {
       if(Statistics.Status_devices_in_level_1[i1]!=0)
       {
        //����� ���������� ����������
        Statistics.Number_Queue=i1;
        i2=i1;
        i1=NUMBER_OF_DEVICES;
        Statistics.Status_devices_in_level_1[Statistics.Number_Queue]++;
        pBuf->Number_Destination=Statistics.List_devices_in_level_1[Statistics.Number_Queue];
        Statistics.Stage=STAGE_RECEIVING_ANSWER_SURVEY_DEVICE_IN_LEVEL_PLUS;
        Statistics.time_Stage=0;
        Statistics.time_Stage_limit=SIGMA_WAIT_ANSWER_SURVEY_DEVICES_IN_LEVEL_PLUS;
       }
       i1++;
      }
      if(i2==NUMBER_OF_DEVICES)
      {
       //����� ��������� ��������� �������� ��������� � ������ �����
       pBuf->Number_Destination=BROADCAST_ADDRESS;
       Statistics.Stage=STAGE_RECEIVING_MESSAGES_FROM_NEW_DEVISES_LEVEL_PLUS;
       Statistics.time_Stage=0;
       Statistics.time_Stage_limit=2+Subscriber.Sigma_send;
      }

      pBuf->Level=Statistics.Level_now;
      pBuf->N_Paketa=NumberPacket; NumberPacket++;
      pBuf->Status=0;
      pBuf->TTL=1;

      PacketBuf[INDX_PCT_BUF_0].repeat=1;
      PacketBuf[INDX_PCT_BUF_0].len=sizeof(HPacket)+1;
      PacketBuf[INDX_PCT_BUF_0].buf[(PacketBuf[INDX_PCT_BUF_0].len-1)]=STOP_BYTE;
      PacketBuf[INDX_PCT_BUF_0].time_delta_for_send=0;
      PacketBuf[INDX_PCT_BUF_0].time_tmp=0;
  break;

  //��������� ������ ������� �� ����� ��������� � Level+1
  case STAGE_RECEIVING_MESSAGES_FROM_NEW_DEVISES_LEVEL_PLUS:
      if(Statistics.time_Stage>Statistics.time_Stage_limit)
      {
       if(Subscriber.Number_Module==1)
       {
        //��������� �������� ����� � ����� ������������
        Statistics.Stage=STAGE_PROCESSING_ARCHIVE;
        Statistics.counter_Stage_Processing_Archive=SIZE_PCTBUF-1;//MAX_PACKET_SEND_PROCESSING_ARCHIVE;
        Statistics.Number_Queue=0;//��� �������� � ��������� ���������� ��� ������
       }
       else
       {
        //��������� �������� �����
        Statistics.Stage=STAGE_PROCESSING_ARCHIVE;
        Statistics.counter_Stage_Processing_Archive=MAX_PACKET_SEND_PROCESSING_ARCHIVE;
        Statistics.Number_Queue=0;

        //���������� ���� ������ ����� ��������� � Level+1, ������ ������� �����, ����� ����
        //���������� ���� �����, ����� ������ ������ ����� ���������� ��������� ����������
        pBuf=(pHPacket)&PacketBuf[INDX_PCT_BUF_0].buf;
        pBuf->Start=START_BYTE;
        pBuf->Type_Comand=TYPE_COMAND_ARCHIVE;
        pBuf->Number_Source=0;
        pBuf->Number_Destination=Statistics.Router_Down;
        pBuf->Level=Statistics.Level_now;
        pBuf->N_Paketa=NumberPacket; NumberPacket++;
        pBuf->Status=0;
        pBuf->Status|=TYPE_PACKET_NULL;
        pBuf->TTL=1;

        PacketBuf[INDX_PCT_BUF_0].repeat=1;
        PacketBuf[INDX_PCT_BUF_0].len=sizeof(HPacket)+1;
        PacketBuf[INDX_PCT_BUF_0].buf[(PacketBuf[INDX_PCT_BUF_0].len-1)]=STOP_BYTE;
        PacketBuf[INDX_PCT_BUF_0].time_delta_for_send=0;
        PacketBuf[INDX_PCT_BUF_0].time_tmp=0;
       }
      }
  break;

  case STAGE_RECEIVING_ANSWER_SURVEY_DEVICE_IN_LEVEL_PLUS:
      if(Statistics.time_Stage>Statistics.time_Stage_limit)
      {
       //����� �����������, � ������ ���. ��������� � ������ ���������� ����������
       if(Statistics.Number_Queue<NUMBER_OF_DEVICES)
       {
        if(Statistics.Status_devices_in_level_1[Statistics.Number_Queue]>LIMIT_TRUST) Statistics.Status_devices_in_level_1[Statistics.Number_Queue]=0;
       }
       Statistics.Number_Queue++;//�������� ��� ������ ������ ����������
       Statistics.Stage=STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS;
      }
  break;
 }

 if(Mailbox_pend(mbxHandleRFtx, &msg_uart, BIOS_NO_WAIT))
 {
  //��������� ����� �������������� ������� �� uart
  if((Subscriber.Number_Module!=1) && (msg_uart.len<PACKET_SIZE_MAX))
  {
   //���� ��������� ������
   i1=0;
   while(i1<(SIZE_PCTBUF-1))
   {
    if(PacketBuf[i1].repeat==0)
    {
     indxPctBuf=i1;//��������� ������, ����� ��������� �� �����������
     i1=SIZE_PCTBUF;
    }
    i1++;
   }

   //��������� ����� ������ � ������ �������. ���� ��� �� ������, �� ���������� � ��������� ����� ���� ������ � ������ ��� ������,
   if(PacketBuf[INDX_PCT_BUF_0].len==(sizeof(HPacket)+1))
   {
    if(msg_uart.len>sizeof(HPacket))
    {
     //��������� � ���� ������ �������� ������
     pBuf=(pHPacket)&PacketBuf[INDX_PCT_BUF_0].buf;
     memcpy(&PacketBuf[INDX_PCT_BUF_0].buf[sizeof(HPacket)], &msg_uart.buf[sizeof(HPacket)], (msg_uart.len-sizeof(HPacket)));
     PacketBuf[INDX_PCT_BUF_0].len=msg_uart.len;
    }
   }
   else
   {
    if(msg_uart.len>sizeof(HPacket))
    {
     pBuf=(pHPacket)&PacketBuf[indxPctBuf].buf;
     pBuf->Start=START_BYTE;
     pBuf->Type_Comand=TYPE_COMAND_ARCHIVE;
     pBuf->Number_Source=0;
     pBuf->Number_Destination=Statistics.Router_Down;
     pBuf->Level=Statistics.Level_now;
     pBuf->N_Paketa=NumberPacket; NumberPacket++;
     pBuf->Status=0;
     pBuf->TTL=1;

     memcpy(&PacketBuf[indxPctBuf].buf[sizeof(HPacket)], &msg_uart.buf[sizeof(HPacket)], (msg_uart.len-sizeof(HPacket)));

     PacketBuf[indxPctBuf].repeat=Subscriber.Repeat;
     PacketBuf[indxPctBuf].len=msg_uart.len;
     PacketBuf[indxPctBuf].time_delta_for_send=0;
     PacketBuf[indxPctBuf].time_tmp=1;
     indxPctBuf++;
     if(indxPctBuf>=(SIZE_PCTBUF-1)) indxPctBuf=0;
    }
   }
   pBuf_from_UART=(pHPacket)&msg_uart.buf;
   if(Subscriber.Number_Module!=pBuf_from_UART->Number_Source) srand(pBuf_from_UART->Number_Source);
   Subscriber.Number_Module=pBuf_from_UART->Number_Source;
   pBuf->Number_Source=Subscriber.Number_Module;
   pBuf->Status|=pBuf_from_UART->Status;
  }
 }

 //������ ����� �������
 if(CounterTrust>4001)
 {
  CounterTrust=0;
  if(Statistics.Router_Down_Counter_Trust) Statistics.Router_Down_Counter_Trust--;//����������
  {
   //������� � ������� ������� �����������, �� ���� �� ��������� ����� 3 �
   if(Statistics.Router_Down_Counter_Trust && Statistics.Level_now) i2=Statistics.Level_now-1;//����� � ������ �������� �� ��������, ������� ����� ������������� ���� ������ ���� �������� ����� �������� � ����� ������� Level
   else i2=0xFF;
   for(i1=0; i1<MAX_SIZE_HANDOVER_LIST; i1++)
   {
    if(Handover[i1].Counter>(LIMIT_HANDOVER_TRUST-1))
    {
     if(Handover[i1].Level<i2)
     {
      i2=Handover[i1].Level;
      Statistics.Router_Down=Handover[i1].Router_Down;
      Statistics.Router_Down_Counter_Trust=0;
     }
    }
   }
  }

  for(i1=0; i1<MAX_SIZE_HANDOVER_LIST; i1++)
  {
   //����������� ��������� ������� � �������� �� ������ handover, ��� ������� ������� ���������� �������,
   if(Handover[i1].Counter) Handover[i1].Counter--;
  }
 }

 for(i1=0; i1<SIZE_PCTBUF; i1++) PacketBuf[i1].time_tmp++;//���������� ������� � ������� ������ ��������

 if(Statistics.Stage==STAGE_PROCESSING_ARCHIVE)
 {
  if(Statistics.counter_Stage_Processing_Archive)
  {
   i1=indx_last_packet;//��������� �������� ������ �� ������ ������ ������� � ���������� �������
   Statistics.counter_Stage_Processing_Archive--;
  }
  else
  {
   //���������� ����� ������������ ���������� ������� �� ������
   if(Subscriber.Number_Module==1)
   {
    Statistics.Stage=STAGE_SURVEY_DEVICES_IN_LEVEL_PLUS;
    i1=SIZE_PCTBUF;//������ �� ����������
   }
   else i1=SIZE_PCTBUF-1;//���������� ���� �����.
  }
 }
 else i1=SIZE_PCTBUF-1;//���������� ������ �������, � ��� ����� ���� �����.

 while(i1<SIZE_PCTBUF)//�������� �� �������� ������
 {
  pBuf=(pHPacket)&PacketBuf[i1].buf;
  if((PacketBuf[i1].repeat) && (pBuf->TTL))
  {
   if(PacketBuf[i1].time_tmp>=PacketBuf[i1].time_delta_for_send)
   {
    PacketBuf[i1].repeat--;
    if(pBuf->Type_Comand==TYPE_COMAND_ARCHIVE)
    {
     time_now+=MULTI_K;//���� ������ ������ �����, ������� ����������� ������� ���������� �������
     PacketBuf[i1].time_delta_for_send=PacketBuf[i1].time_tmp+MULTI_K*MAX_PACKET_SEND_PROCESSING_ARCHIVE*2;//����� ������� ����� ��� �������� ����� �� ������� �������, � �� ���������� ����� ��� ������ � ������ ����� ������
    }
    //��������� � ������� ������ ����� ���������� �� �������, �.�. ������ � ������ ����� �����, � ����� ������� ������� ��� ���������� ��� ���� ������ ���� (� ����� ������� ��� ��������� ����������)
    if((Statistics.Stage==STAGE_PROCESSING_ARCHIVE) && (i1!=(SIZE_PCTBUF-1))) pBuf->Number_Destination=Statistics.Router_Down;
    if((Statistics.Stage==STAGE_PROCESSING_ARCHIVE) && (i1==(SIZE_PCTBUF-1)))
    {
     Statistics.Stage=STAGE_IDLE;
    }
    indx_last_packet=i1+1;//��� ������������ ����������� ������
    if(indx_last_packet>=(SIZE_PCTBUF-1)) indx_last_packet=0;
 
    *pBuf_sending_packages=PacketBuf[i1].buf; //����� �� ��������
    if(Subscriber.Number_Module) *pLen_sending_packages=PacketBuf[i1].len;
    i1=SIZE_PCTBUF;
   }
  }
  i1++;
 }
 if(i1!=(1+SIZE_PCTBUF)) *pLen_sending_packages=0;
}
