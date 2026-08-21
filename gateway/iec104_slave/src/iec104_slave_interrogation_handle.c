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
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/

#include "iec104_slave_interrogation_handle.h"
#include "iec104_cfg.h"
#include "db_node_fops.h"


/*********************************************************************************************************
** Function name:
** Descriptions:        浮点
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_interrogation_valueShort(CS101_AppLayerParameters alParams, IMasterConnection connection, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_NUM_MAX       20

    int j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueShort io;

    struct config_chnl_node_attr    *pNode_attr;

    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {

        // 单帧最大不超过20
        if(j == 0)
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
            io = (MeasuredValueShort) MeasuredValueShort_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShort_create((MeasuredValueShort) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
        j++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_NUM_MAX) || (k == pNode_group->node_num))
        {
            j = 0;
            MeasuredValueShort_destroy(io);
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);
        }
    }   

    if(k != pNode_group->node_num)
    {
        MeasuredValueShort_destroy(io); 
        CS101_ASDU_destroy(newAsdu);
    }
    
    return 0;
}



/*********************************************************************************************************
** Function name:
** Descriptions:        归一化
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_interrogation_valueScale(CS101_AppLayerParameters alParams, IMasterConnection connection, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_NUM_MAX       20

    int j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueScaled io;

    struct config_chnl_node_attr    *pNode_attr;

    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20
        if(j == 0)
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
            io = (MeasuredValueScaled) MeasuredValueScaled_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueScaled_create((MeasuredValueScaled) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
        j++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_NUM_MAX) || (k == pNode_group->node_num))
        {
            j = 0;
            MeasuredValueScaled_destroy(io);
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);
        }
    }    

    if(k != pNode_group->node_num)
    {
        MeasuredValueScaled_destroy(io); 
        CS101_ASDU_destroy(newAsdu);

    }

    
    return 0;
}



/*********************************************************************************************************
** Function name:
** Descriptions:        浮点
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_interrogation_MeasuredValue(CS101_AppLayerParameters alParams, IMasterConnection connection, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_NUM_MAX       20

    int j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueNormalized io;

    struct config_chnl_node_attr    *pNode_attr;

    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {

        // 单帧最大不超过20
        if(j == 0)
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
            io = (MeasuredValueNormalized) MeasuredValueNormalized_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueNormalized_create((MeasuredValueNormalized) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
        j++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_NUM_MAX) || (k == pNode_group->node_num))
        {
            j = 0;
            MeasuredValueNormalized_destroy(io);
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);
        }
    }   

    if(k != pNode_group->node_num)
    {
        MeasuredValueNormalized_destroy(io); 
        CS101_ASDU_destroy(newAsdu); 
    }
    

    return 0;
}



/*********************************************************************************************************
** Function name:
** Descriptions:        单点 
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_interrogation_Singlepoint(CS101_AppLayerParameters alParams, IMasterConnection connection, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_NUM_MAX       20

    int j, k;
    double lfval;
    CS101_ASDU newAsdu;
    SinglePointInformation io;

    struct config_chnl_node_attr    *pNode_attr;

    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20
        if(j == 0)
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_INTERROGATED_BY_STATION, 0, 1, false, false);
            io = (SinglePointInformation) SinglePointInformation_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);

        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
        j++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_NUM_MAX) || (k == pNode_group->node_num))
        {
            j = 0;
            SinglePointInformation_destroy(io);
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);
        }
    }    

    if(k != pNode_group->node_num)
    {
        SinglePointInformation_destroy(io); 
        CS101_ASDU_destroy(newAsdu);
    }

    return 0;
}



/*********************************************************************************************************
** Function name:
** Descriptions:        浮点
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_counterinterrogation(CS101_AppLayerParameters alParams, IMasterConnection connection, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_NUM_MAX       20

    int j, k;
    double lfval;
    CS101_ASDU newAsdu;
    IntegratedTotals io;

    struct config_chnl_node_attr    *pNode_attr;
    BinaryCounterReading             bcr;

    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20
        if(j == 0)
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_REQUESTED_BY_GENERAL_COUNTER, 0, 1, false, false);
            bcr = BinaryCounterReading_create(NULL, -1, -1, false, false, false);
            io =  IntegratedTotals_create(NULL, addr + pNode_attr->reg_no, bcr);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);

        bcr = BinaryCounterReading_create(bcr, lfval, -1, false, false, false);
        CS101_ASDU_addInformationObject(newAsdu, (InformationObject) IntegratedTotals_create((IntegratedTotals) io, addr + pNode_attr->reg_no, bcr));
        j++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_NUM_MAX) || (k == pNode_group->node_num))
        {
            j = 0;
            IntegratedTotals_destroy(io);
            IMasterConnection_sendASDU(connection, newAsdu);
            CS101_ASDU_destroy(newAsdu);
            BinaryCounterReading_destroy(bcr);
        }
    }    

    if(k != pNode_group->node_num)
    {
        IntegratedTotals_destroy(io);
        CS101_ASDU_destroy(newAsdu);
        BinaryCounterReading_destroy(bcr);
    }
    

    return 0;
}

/*********************************************************************************************************
** Function name:
** Descriptions: 响应主站查询请求，服务端从站需从设备获取数据，并且发送返回给主站端，一般为总召
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_interrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qoi)
{
    CS101_AppLayerParameters alParams;

    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;
    struct config_node_group        *pNode_group ;

    if (qoi == 20) 
    { /* only handle station interrogation */
        alParams = IMasterConnection_getApplicationLayerParameters(connection);

        IMasterConnection_sendACT_CON(connection, asdu, false);

        pNode_group = &pChnl->MMEA;
        iec104_slave_interrogation_MeasuredValue(alParams, connection, pNode_group, pChnl->MMEA.addr);

        pNode_group = &pChnl->MMEB;
        iec104_slave_interrogation_valueScale(alParams, connection, pNode_group, pChnl->MMEB.addr);

        pNode_group = &pChnl->MMEC;
        iec104_slave_interrogation_valueShort(alParams, connection, pNode_group, pChnl->MMEC.addr);

        pNode_group = &pChnl->MSP;
        iec104_slave_interrogation_Singlepoint(alParams, connection, pNode_group, pChnl->MSP.addr);
            
        IMasterConnection_sendACT_TERM(connection, asdu);
    }
    else 
    {
        IMasterConnection_sendACT_CON(connection, asdu, true);
    }

    return true;
}



/*********************************************************************************************************
** Function name:
** Descriptions: 响应主站查询请求，服务端从站需从设备获取数据，并且发送返回给主站端，一般为总召
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_counterInterrogationHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu, uint8_t qcc)
{
    CS101_AppLayerParameters alParams;

    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;
    struct config_node_group        *pNode_group ;


//    if (qcc == 5) 
    { /* only handle station interrogation */
        alParams = IMasterConnection_getApplicationLayerParameters(connection);

        IMasterConnection_sendACT_CON(connection, asdu, false);

        pNode_group = &pChnl->CCI;
        iec104_slave_counterinterrogation(alParams, connection, pNode_group, pChnl->CCI.addr);
            
        IMasterConnection_sendACT_TERM(connection, asdu);
    }
//    else 
//    {
//        IMasterConnection_sendACT_CON(connection, asdu, true);
//    }

    return true;
}

