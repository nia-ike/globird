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

#include <math.h>

#include "iec104_slave_spontaneous.h"
#include "iec104_cfg.h"
#include "db_node_fops.h"
#include "cs104_slave.h"


#define SPONTANEOUS_DIV         0.001

/*********************************************************************************************************
** Function name:
** Descriptions:        浮点
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int iec104_slave_spontaneous_valueShort(struct config_chnl_working_attr *pChnl, CS101_AppLayerParameters alParams, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_VALUESHORT_NUM_MAX       30

    int i, j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueShort io;

    struct config_chnl_node_attr    *pNode_attr;

    i = 0;
    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20, 一帧开始 申请资源
        if((j == 0) && (i == 0))
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
            io = (MeasuredValueShort) MeasuredValueShort_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);

        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        if(fabs(lfval - pNode_attr->lastvalue) > SPONTANEOUS_DIV)
        {
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueShort_create((MeasuredValueShort) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
            j++;

            /* update value */
            pNode_attr->lastvalue = lfval;
        }
        
        i++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_VALUESHORT_NUM_MAX) || (k == pNode_group->node_num))
        {
            
            MeasuredValueShort_destroy(io);
            if(j)
                CS104_Slave_enqueueASDU(pChnl->iec104_comm_ctx.slave, newAsdu);
            CS101_ASDU_destroy(newAsdu);
            j = 0;
            i = 0;
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
static int iec104_slave_spontaneous_valueScale(struct config_chnl_working_attr *pChnl, CS101_AppLayerParameters alParams, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_VALUESCALE_NUM_MAX       20

    int i, j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueScaled io;

    struct config_chnl_node_attr    *pNode_attr;

    i = 0;
    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20, 一帧开始 申请资源
        if((j == 0) && (i == 0))
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
            io = (MeasuredValueScaled) MeasuredValueScaled_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        if(fabs(lfval - pNode_attr->lastvalue) > SPONTANEOUS_DIV)
        {
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueScaled_create((MeasuredValueScaled) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
            j++;
            /* update value */
            pNode_attr->lastvalue = lfval;
        }

        i++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_VALUESCALE_NUM_MAX) || (k == pNode_group->node_num))
        {
            MeasuredValueScaled_destroy(io);
            if(j)
                CS104_Slave_enqueueASDU(pChnl->iec104_comm_ctx.slave, newAsdu);
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
static int iec104_slave_spontaneous_MeasuredValue(struct config_chnl_working_attr *pChnl, CS101_AppLayerParameters alParams, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_MEASUREVALUE_NUM_MAX       30

    int i, j, k;
    double lfval;
    CS101_ASDU newAsdu;
    MeasuredValueNormalized io;

    struct config_chnl_node_attr    *pNode_attr;

    i = 0;
    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20, 一帧开始 申请资源
        if((j == 0) && (i == 0))
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
            io = (MeasuredValueNormalized) MeasuredValueNormalized_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);
        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        if(fabs(lfval - pNode_attr->lastvalue) > SPONTANEOUS_DIV)
        {
            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) MeasuredValueNormalized_create((MeasuredValueNormalized) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
            j++;
            /* update value */
            pNode_attr->lastvalue = lfval;
        }

        i++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_MEASUREVALUE_NUM_MAX) || (k == pNode_group->node_num))
        {
            MeasuredValueNormalized_destroy(io);
            if(j)
                CS104_Slave_enqueueASDU(pChnl->iec104_comm_ctx.slave, newAsdu);
            CS101_ASDU_destroy(newAsdu);

            j = 0;
            i = 0;
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
static int iec104_slave_spontaneous_Singlepoint(struct config_chnl_working_attr *pChnl, CS101_AppLayerParameters alParams, struct config_node_group *pNode_group, uint32_t addr)
{
#define INTERROGA_SINGLEPOINT_NUM_MAX       30

    int i, j, k;
    double lfval;
    CS101_ASDU newAsdu;
    SinglePointInformation io;

    struct config_chnl_node_attr    *pNode_attr;

    i = 0;
    k = 0;
    j = 0;
    slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
    {
        // 单帧最大不超过20, 一帧开始 申请资源
        if((j == 0) && (i == 0))
        {
            newAsdu = CS101_ASDU_create(alParams, false, CS101_COT_SPONTANEOUS, 0, 1, false, false);
            io = (SinglePointInformation) SinglePointInformation_create(NULL, addr + pNode_attr->reg_no, -1, IEC60870_QUALITY_GOOD);

        }

        DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);
        if(fabs(lfval - pNode_attr->lastvalue) > SPONTANEOUS_DIV)
        {

            CS101_ASDU_addInformationObject(newAsdu, (InformationObject) SinglePointInformation_create((SinglePointInformation) io, addr + pNode_attr->reg_no, lfval, IEC60870_QUALITY_GOOD));
          
            j++;
            /* update value */
            pNode_attr->lastvalue = lfval;
        }

        i++;
        k++;
        // 达到单帧最大长度，或者最后数据不足1帧
        if((j == INTERROGA_SINGLEPOINT_NUM_MAX) || (k == pNode_group->node_num))
        {
            
            SinglePointInformation_destroy(io);
            if(j)
                CS104_Slave_enqueueASDU(pChnl->iec104_comm_ctx.slave, newAsdu);
            CS101_ASDU_destroy(newAsdu);
            j = 0;
            i = 0;
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
** Descriptions: 响应主站查询请求，服务端从站需从设备获取数据，并且发送返回给主站端，一般为总召
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_spontaneous_Handler(struct config_chnl_working_attr *pChnl)
{
    CS101_AppLayerParameters alParams;

    struct config_node_group        *pNode_group ;

    alParams = CS104_Slave_getAppLayerParameters(pChnl->iec104_comm_ctx.slave);

    { /* only handle station interrogation */

        pNode_group = &pChnl->MMEA;
        iec104_slave_spontaneous_MeasuredValue(pChnl, alParams, pNode_group, pChnl->MMEA.addr);

        pNode_group = &pChnl->MMEB;
        iec104_slave_spontaneous_valueScale(pChnl,alParams, pNode_group, pChnl->MMEB.addr);//

        pNode_group = &pChnl->MMEC;
        iec104_slave_spontaneous_valueShort(pChnl, alParams, pNode_group, pChnl->MMEC.addr);//
 
        pNode_group = &pChnl->MSP;
        iec104_slave_spontaneous_Singlepoint(pChnl,alParams, pNode_group, pChnl->MSP.addr);   

    }


    return true;
}



