/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: 
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/

#include "iec104_asdu_packetAndSend.h"

#if 0

/*
IEC104协议：APDU=APCI+ASDU=255
APCI长度为6字节。U帧和S帧不包括ASDU。
ASDU长度最大为249字节，信息体长度最大为243字节。
信息体地址长度为3字节
短浮点测量值由4字节浮点数+1字节品质描述词=5字节

长时标7字节，短时标3字节

对于信息体（最大243字节）：
总召时（连续地址），归一化AI一帧最大发送80个数据 3字节地址+80*3字节数据 = 243字节+12=255
        标度化AI一帧最大发送80个数据 3字节地址+80*3字节数据= 243字节+12=255
        浮点型AI一帧最大发送48个数据 3字节地址+48*5字节数据 = 243字节+12=255
        DI一帧最大发送127个数据  3字节地址+127字节数据 = 130字节+12=142

总召时（不连续地址），浮点型AI一帧最大发送30个数据 3字节地址+30*8字节数据= 243字节+12=255
       DI一帧最大发送60个数据 3字节地址+60*4字节数据数据=243字节+12=255

变化时（不连续地址），AI 带CP56Time2a时标的 一次最多传送16个 (16*13+12)=246字节 或 (16*15+12)=252字节
        AI 不带时标或者不带品质的一次最多上传30个  (30*8+12)=252字节 或 (30*5+12)=162字节 或 (30*6+12)=192字节
        DI带iec104长时标的，一次最多上传22个变化量(22*11+12)=254字节
        DI带短时标的，一次最多上传32个变化量(32*7+12)=236字节
*/

/*********************************************************************************************************
** 发送短浮点测量值
**  
** 
** 
** 
*********************************************************************************************************/
void iec104_slave_measuredValueShortASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,CS101_CauseOfTransmission cot)
{
    uint8_t i;
    int addr;

    //发送asdu, 其中COT设置为CS101_COT_INTERROGATED_BY_STATION，从站总召
    CS101_ASDU newAsdu;
    InformationObject io;
    switch(cot)
    {
        case CS101_COT_INTERROGATED_BY_STATION:
                 newAsdu = CS101_ASDU_create(alParams, false, cot, 0, 1, false, false); //第二个参数false表示不是连续地址
                 //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                 io = (InformationObject) MeasuredValueShort_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD);
                 CS101_ASDU_addInformationObject(newAsdu, io );   
                 pNode++;          
                 for(i=1;i<nodeNum;i++)
                 {
                     if(i%30==0)//一帧最多传30个点数据
                     {

                        InformationObject_destroy(io);
                        //总召发送使用
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                        
                        newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                        //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                        io = (InformationObject) MeasuredValueShort_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD);
                        CS101_ASDU_addInformationObject(newAsdu, io );   
                        pNode++;
                     }else{
                         CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShort_create((MeasuredValueShort) io, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD));
                         pNode++;
                     }
                 }
                 InformationObject_destroy(io);
                 //总召发送使用
                 IMasterConnection_sendASDU(connection, newAsdu);
                 CS101_ASDU_destroy(newAsdu);
             break;
        case CS101_COT_PERIODIC://周期
        case CS101_COT_SPONTANEOUS://突发、自发
                 newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                 //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                 io = (InformationObject) MeasuredValueShort_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD);
                 CS101_ASDU_addInformationObject(newAsdu, io );   
                 pNode++;          
                for(i=1;i<nodeNum;i++)
                 {
                     if(i%30==0)//一帧最多传30个点数据
                     {

                        InformationObject_destroy(io);
                        /* Add ASDU to slave event queue  将asdu放入数据队列进行传输，周期、突发（自发）使用*/
                        CS104_Slave_enqueueASDU(slave, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                        
                        newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                        //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                        io = (InformationObject) MeasuredValueShort_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD);
                        CS101_ASDU_addInformationObject(newAsdu, io );   
                        pNode++;
                     }else{
                         CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShort_create((MeasuredValueShort) io, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD));
                         pNode++;
                     }
                 }
                 InformationObject_destroy(io);
                 /* Add ASDU to slave event queue  将asdu放入数据队列进行传输，周期、突发（自发）使用*/
                 CS104_Slave_enqueueASDU(slave, newAsdu);
                 CS101_ASDU_destroy(newAsdu);
                 break;
        default:
                 break;
    }

}
/*********************************************************************************************************
** 发送单点信息
**  
** 
** 
** 
*********************************************************************************************************/
void iec104_slave_singlePointASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,CS101_CauseOfTransmission cot)
{
    printf("%s:%d:%s\n",__FILE__,__FILE__,__func__);
    uint8_t i;
    int addr;
    bool value;
    printf("DI nodeNum=%d\n",nodeNum);


    //发送asdu, 其中COT设置为CS101_COT_INTERROGATED_BY_STATION，从站总召
    CS101_ASDU newAsdu;
    InformationObject io;
    switch(cot)
    {
        case CS101_COT_INTERROGATED_BY_STATION:
            newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
            if(pNode->node_value==(float)1)
            {
                value=true;
            }else
            {
                value=false;
            }
            io = (InformationObject) SinglePointInformation_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD);
            CS101_ASDU_addInformationObject(newAsdu, io );   
            pNode++;          
            for(i=1;i<nodeNum;i++)
            {
                if(i%60==0)//一帧最多传60个点数据
                {
                    InformationObject_destroy(io);
                    //总召发送使用
                    IMasterConnection_sendASDU(connection, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                        
                    newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                    io = (InformationObject) SinglePointInformation_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD);
                    CS101_ASDU_addInformationObject(newAsdu, io );   
                    pNode++;
                }else{
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, pNode->node_addr, value, IEC60870_QUALITY_GOOD));
                    pNode++;
                }
            }
            InformationObject_destroy(io);
            //总召发送使用
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);         
            break;
        case CS101_COT_PERIODIC://周期
        case CS101_COT_SPONTANEOUS://突发、自发
            newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
            if(pNode->node_value==(float)1)
            {
                value=true;
            }else
            {
                value=false;
            }
            io = (InformationObject) SinglePointInformation_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD);
            CS101_ASDU_addInformationObject(newAsdu, io );   
            pNode++;          
            for(i=1;i<nodeNum;i++)
            {
                if(i%60==0)//一帧最多传60个点数据
                {
                    InformationObject_destroy(io);
                    CS104_Slave_enqueueASDU(slave, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                        
                    newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                    io = (InformationObject) SinglePointInformation_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD);
                    CS101_ASDU_addInformationObject(newAsdu, io );   
                    pNode++;
                }else{
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, pNode->node_addr, value, IEC60870_QUALITY_GOOD));
                    pNode++;
                }
            }
            InformationObject_destroy(io);
            CS104_Slave_enqueueASDU(slave, newAsdu);
            CS101_ASDU_destroy(newAsdu);   
            break;
        default:
            break;
    }
}
/*********************************************************************************************************
** 发送短浮点测量值,带长时标
**  
** 
** 
** 
*********************************************************************************************************/
void iec104_slave_CP56Time2a_measuredValueShortASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,uint64_t timestamp,CS101_CauseOfTransmission cot)
{
    printf("%s:%d:%s\n",__FILE__,__FILE__,__func__);
    uint8_t i;
    int addr;
    printf("nodeNum=%d\n",nodeNum);
    struct sCP56Time2a cpTime;
    CP56Time2a_createFromMsTimestamp(&cpTime, timestamp);

    //发送asdu, 其中COT设置为CS101_COT_INTERROGATED_BY_STATION，从站总召
    CS101_ASDU newAsdu;
    InformationObject io;
    switch(cot)
    {
        case CS101_COT_INTERROGATED_BY_STATION:
                 newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                 //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                 io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime);
                 CS101_ASDU_addInformationObject(newAsdu, io );   
                 pNode++;          
                 for(i=1;i<nodeNum;i++)
                 {
                     if(i%16==0)//一帧最多传16个点数据
                     {

                        InformationObject_destroy(io);
                        //总召发送使用
                        IMasterConnection_sendASDU(connection, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                        
                        newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                        //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                        io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime);
                        CS101_ASDU_addInformationObject(newAsdu, io );   
                        pNode++;
                     }else{
                         CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShortWithCP56Time2a_create((MeasuredValueShortWithCP56Time2a) io, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime));
                         pNode++;
                     }
                 }
                 InformationObject_destroy(io);
                 //总召发送使用
                 IMasterConnection_sendASDU(connection, newAsdu);
                 CS101_ASDU_destroy(newAsdu);
             break;
        case CS101_COT_PERIODIC://周期
        case CS101_COT_SPONTANEOUS://突发、自发
        printf("#######CP56Time2a:CS101_COT_SPONTANEOUS####### \n");
                 newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                 //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                 io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime);
                 CS101_ASDU_addInformationObject(newAsdu, io );   
                 pNode++;          
                for(i=1;i<nodeNum;i++)
                 {
                     if(i%16==0)//一帧最多传16个点数据
                     {

                        InformationObject_destroy(io);
                        /* Add ASDU to slave event queue  将asdu放入数据队列进行传输，周期、突发（自发）使用*/
                        CS104_Slave_enqueueASDU(slave, newAsdu);
                        CS101_ASDU_destroy(newAsdu);
                        
                        newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址,变位使用不连续地址方式发送
                        //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                        io = (InformationObject) MeasuredValueShortWithCP56Time2a_create(NULL, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime);
                        CS101_ASDU_addInformationObject(newAsdu, io );   
                        pNode++;
                     }else{
                         CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShortWithCP56Time2a_create((MeasuredValueShortWithCP56Time2a) io, pNode->node_addr, pNode->node_value, IEC60870_QUALITY_GOOD,&cpTime));
                         pNode++;
                     }
                 }
                 InformationObject_destroy(io);
                 /* Add ASDU to slave event queue  将asdu放入数据队列进行传输，周期、突发（自发）使用*/
                 CS104_Slave_enqueueASDU(slave, newAsdu);
                 CS101_ASDU_destroy(newAsdu);
                 break;
        default:
                 break;
    }

}

/*********************************************************************************************************
** 发送短浮点测量值,带长时标
**  
** 
** 
** 
*********************************************************************************************************/
void iec104_slave_CP56Time2a_singlePointASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave,struct cs104_node_data* pNode,int nodeNum,uint64_t timestamp,CS101_CauseOfTransmission cot)
{
    printf("%s:%d:%s\n",__FILE__,__FILE__,__func__);
    uint8_t i;
    int addr;
    bool value;
    printf("DI nodeNum=%d\n",nodeNum);
    struct sCP56Time2a cpTime;
    CP56Time2a_createFromMsTimestamp(&cpTime, timestamp);

    //发送asdu, 其中COT设置为CS101_COT_INTERROGATED_BY_STATION，从站总召
    CS101_ASDU newAsdu;
    InformationObject io;
    switch(cot)
    {
        case CS101_COT_INTERROGATED_BY_STATION:
            newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
            if(pNode->node_value==(float)1)
            {
                value=true;
            }else
            {
                value=false;
            }
            io = (InformationObject) SinglePointWithCP56Time2a_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime);
            CS101_ASDU_addInformationObject(newAsdu, io );   
            pNode++;          
            for(i=1;i<nodeNum;i++)
            {
                if(i%22==0)//一帧最多传22个点数据
                {
                    InformationObject_destroy(io);
                    //总召发送使用
                    IMasterConnection_sendASDU(connection, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                        
                    newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                    io = (InformationObject) SinglePointWithCP56Time2a_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime);
                    CS101_ASDU_addInformationObject(newAsdu, io );   
                    pNode++;
                }else{
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointWithCP56Time2a_create((SinglePointWithCP56Time2a) io, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime));
                    pNode++;
                }
            }
            InformationObject_destroy(io);
            //总召发送使用
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);           
            break;
        case CS101_COT_PERIODIC://周期
        case CS101_COT_SPONTANEOUS://突发、自发
            newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
            if(pNode->node_value==(float)1)
            {
                value=true;
            }else
            {
                value=false;
            }
            io = (InformationObject) SinglePointWithCP56Time2a_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime);
            CS101_ASDU_addInformationObject(newAsdu, io );   
            pNode++;          
            for(i=1;i<nodeNum;i++)
            {
                if(i%22==0)//一帧最多传22个点数据
                {
                    InformationObject_destroy(io);
                    CS104_Slave_enqueueASDU(slave, newAsdu);
                    CS101_ASDU_destroy(newAsdu);
                        
                    newAsdu = CS101_ASDU_create(alParams, false, cot,0, 1, false, false); //第二个参数false表示不是连续地址
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    //创建信息体对象，并且把第一个点信息（地址+值）增加到信息体中
                    io = (InformationObject) SinglePointWithCP56Time2a_create(NULL, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime);
                    CS101_ASDU_addInformationObject(newAsdu, io );   
                    pNode++;
                }else{
                    if(pNode->node_value==(float)1)
                    {
                        value=true;
                    }else
                    {
                        value=false;
                    }
                    CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointWithCP56Time2a_create((SinglePointWithCP56Time2a) io, pNode->node_addr, value, IEC60870_QUALITY_GOOD,&cpTime));
                    pNode++;
                }
            } 
            InformationObject_destroy(io);
            /* Add ASDU to slave event queue  将asdu放入数据队列进行传输，周期、突发（自发）使用*/
            CS104_Slave_enqueueASDU(slave, newAsdu);
            CS101_ASDU_destroy(newAsdu);
            break;
        default:
            break;
    }    
}
/*********************************************************************************************************
** Function name:
** Descriptions: 响应主站，总召
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void iec104_slave_ASDUSend(CS101_AppLayerParameters alParams,IMasterConnection connection,CS104_Slave slave, ST_FW_NODE_ATTR* pNodeAttr,CS101_CauseOfTransmission cot)
{
    int i;
    int array_count;
    array_count=cs104_getFwNodeSectionCnt();

    //printf("array_count=%d\n",array_count);
    for(i=0;i<array_count;i++)
    {   
        switch(pNodeAttr[i].nodeType)
        {
            case IEC104_TYPE_DI://遥信,先发遥信
                if(pNodeAttr[i].nodeNum!=0)
                {
                     if(pNodeAttr[i].timestamp==0 || cot==CS101_COT_INTERROGATED_BY_STATION)
                     {
                         singlePointASDUSend(alParams,connection,slave,pNodeAttr[i].pNodeData,pNodeAttr[i].nodeNum,cot);
                     }else{
                         CP56Time2a_singlePointASDUSend(alParams,connection,slave,pNodeAttr[i].pNodeData,pNodeAttr[i].nodeNum,pNodeAttr[i].timestamp,cot);
                     }
                }
                 break;

            case IEC104_TYPE_AI://遥测
                 if(pNodeAttr[i].nodeNum!=0)
                 {
                     if(pNodeAttr[i].timestamp==0 || cot==CS101_COT_INTERROGATED_BY_STATION)
                     {
                         measuredValueShortASDUSend(alParams,connection,slave,pNodeAttr[i].pNodeData,pNodeAttr[i].nodeNum,cot);
                     }else{
                         CP56Time2a_measuredValueShortASDUSend(alParams,connection,slave,pNodeAttr[i].pNodeData,pNodeAttr[i].nodeNum,pNodeAttr[i].timestamp,cot);
                     }
                     
                 }                 
                 break;
            default:
                 break;

        }
    }
}


#endif