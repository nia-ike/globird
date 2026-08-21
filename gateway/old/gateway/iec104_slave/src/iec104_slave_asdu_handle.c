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


#include "iec104_slave_asdu_handle.h"
#include "iec104_cfg.h"
#include "db_node_fops.h"

#include <stdio.h>

#define _FC_READ_HOLDING_REGISTERS      0x03
#define _FC_WRITE_COIL_REGISTERS        0x05
#define _FC_WRITE_SINGLE_REGISTERS      0x06
#define _FC_WRITE_MULTIPLE_REGISTERS    0x10

/*********************************************************************************************************
** Function name:
** Descriptions:        单点遥控
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static bool iec104_slave_asdu_C_SC_NA_1(struct config_chnl_working_attr *pChnl, IMasterConnection connection,  CS101_ASDU asdu)
{
    if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {//激活判断
        InformationObject io = CS101_ASDU_getElement(asdu, 0);//获取信息元素

        if (io) {
            int i;
            uint8_t                            chnlno;
            struct db_spl_channel_ctr_t       *pctrl;
            struct config_chnl_node_attr      *pNode_attr;
            struct config_node_group          *pNode_group;
            struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
            int ioa = InformationObject_getObjectAddress(io);
            
            i = 0;
            pNode_group = &pChnl->CSC;
            slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
            {
                if(pChnl->debug)
                    printf("Find IOA: %i from MAP, Now is %i\n", ioa,
                        pNode_group->addr + i);//SCS，0代表分/开，1代表合/关

                if((pNode_group->addr + i) == ioa)
                {
                    SingleCommand sc = (SingleCommand) io;

                    /*  */
                    if(SingleCommand_isSelect(sc) == false)
                    {
                        /* 获取当前转发点属性中，根据数据类型 把寄存器的值转换成 需要类型的下发值 */
                        chnlno = DB_CONVENT_ID_2_CHNLNO(pNode_attr->db_link) & 0x3f;

                        /* 获取空闲控制信息块（主池满后用 ctrl_extend_region） */
                        pctrl = db_ctrl_alloc_idle(pDB, chnlno);
                        if (pctrl == NULL) {
                            printf("iec104 C_SC_NA_1: control queue full, chnl=%u\n", (unsigned)chnlno);
                            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
                            InformationObject_destroy(io);
                            IMasterConnection_sendASDU(connection, asdu);
                            return true;
                        }
                        /* 
                            基本各种工业协议都支持遥控、遥调 
                            而modbus 是最通用广泛协议，遥控遥调实现最初以modbus为原型
                            因此iec104的控制消息以modbus功能码来作为标识，不失通用性
                            对应的南向协议，如果是modbus协议，自动适配。如果是其他协议（如645协议）
                            会在南向动态库中做转换适配
                        
                        */
                        pctrl->funcode = _FC_WRITE_COIL_REGISTERS;
                        pctrl->is_active = 1;
                        pctrl->db_linkid = pNode_attr->db_link;
                        pctrl->value     = SingleCommand_getState(sc);
                    }

                    CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);//设置激活确认
                    InformationObject_destroy(io);
                    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

                    if(pChnl->debug)
                        printf("IOA: %i switch to %i\n", ioa,
                            SingleCommand_getState(sc));//SCS，0代表分/开，1代表合/关

                    return true;
                }
                i++;
            }

            // can not found the ioa
            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
            InformationObject_destroy(io);
            IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

            return true;

        }
        else {
            printf("ERROR: message has no valid information object\n");
            return true;
        }
    }
    else
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);

    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

    return true;
}




/*********************************************************************************************************
** Function name:
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static bool iec104_slave_asdu_C_SE_NA_1(struct config_chnl_working_attr *pChnl, IMasterConnection connection,  CS101_ASDU asdu)
{
    if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {//激活判断
        InformationObject io = CS101_ASDU_getElement(asdu, 0);//获取信息元素

        if (io) {
            int i;
            uint8_t                            chnlno;
            struct db_spl_channel_ctr_t       *pctrl;
            struct config_chnl_node_attr      *pNode_attr;
            struct config_node_group          *pNode_group;
            struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
            int ioa = InformationObject_getObjectAddress(io);
            
            i = 0;
            pNode_group = &pChnl->CSEA;
            slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
            {
                if(pChnl->debug)
                    printf("Find IOA: %i from MAP, Now is %i\n", ioa,
                        pNode_group->addr + i);//SCS，0代表分/开，1代表合/关

                if((pNode_group->addr + i) == ioa)
                {
                    SetpointCommandNormalized sc = (SetpointCommandNormalized) io;

                    /*  */
                    if(SetpointCommandNormalized_isSelect(sc) == false)
                    {
                        /* 获取当前转发点属性中，根据数据类型 把寄存器的值转换成 需要类型的下发值 */
                        chnlno = DB_CONVENT_ID_2_CHNLNO(pNode_attr->db_link) & 0x3f;

                        /* 获取空闲控制信息块（主池满后用 ctrl_extend_region） */
                        pctrl = db_ctrl_alloc_idle(pDB, chnlno);
                        if (pctrl == NULL) {
                            printf("iec104 C_SE_NA_1: control queue full, chnl=%u\n", (unsigned)chnlno);
                            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
                            InformationObject_destroy(io);
                            IMasterConnection_sendASDU(connection, asdu);
                            return true;
                        }
                        /* 
                            基本各种工业协议都支持遥控、遥调 
                            而modbus 是最通用广泛协议，遥控遥调实现最初以modbus为原型
                            因此iec104的控制消息以modbus功能码来作为标识，不失通用性
                            对应的南向协议，如果是modbus协议，自动适配。如果是其他协议（如645协议）
                            会在南向动态库中做转换适配
                        
                        */
                        pctrl->funcode = _FC_WRITE_MULTIPLE_REGISTERS;
                        pctrl->is_active = 1;
                        pctrl->db_linkid = pNode_attr->db_link;
                        pctrl->value     = SetpointCommandNormalized_getValue(sc);

                    }
                    

                    CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);//设置激活确认
                    InformationObject_destroy(io);
                    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

                    if(pChnl->debug)
                        printf("IOA: %i switch to %f\n", ioa,
                            SetpointCommandNormalized_getValue(sc));//SCS，0代表分/开，1代表合/关

                    return true;
                }
                i++;
            }

            // can not found the ioa
            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
            InformationObject_destroy(io);
            IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

            return true;

        }
        else {
            printf("ERROR: message has no valid information object\n");
            return true;
        }
    }
    else
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);

    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

    return true;
}




/*********************************************************************************************************
** Function name:
** Descriptions:        短浮点设值
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static bool iec104_slave_asdu_C_SE_NC_1(struct config_chnl_working_attr *pChnl, IMasterConnection connection,  CS101_ASDU asdu)
{
    if  (CS101_ASDU_getCOT(asdu) == CS101_COT_ACTIVATION) {//激活判断
        InformationObject io = CS101_ASDU_getElement(asdu, 0);//获取信息元素

        if (io) {
            int i;
            uint8_t                            chnlno;
            struct db_spl_channel_ctr_t       *pctrl;
            struct config_chnl_node_attr      *pNode_attr;
            struct config_node_group          *pNode_group;
            struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
            int ioa = InformationObject_getObjectAddress(io);
            
            i = 0;
            pNode_group = &pChnl->CSEC;
            slist_for_each_entry(pNode_attr, &pNode_group->node_attr.list, list)
            {
                if(pChnl->debug)
                    printf("Find IOA: %i from MAP, Now is %i\n", ioa,
                        pNode_group->addr + i);//SCS，0代表分/开，1代表合/关

                if((pNode_group->addr + i) == ioa)
                {
                    SetpointCommandShort sc = (SetpointCommandShort) io;

                    /*  */
                    if(SetpointCommandShort_isSelect(sc) == false)
                    {
                        /* 获取当前转发点属性中，根据数据类型 把寄存器的值转换成 需要类型的下发值 */
                        chnlno = DB_CONVENT_ID_2_CHNLNO(pNode_attr->db_link) & 0x3f;

                        /* 获取空闲控制信息块（主池满后用 ctrl_extend_region） */
                        pctrl = db_ctrl_alloc_idle(pDB, chnlno);
                        if (pctrl == NULL) {
                            printf("iec104 C_SE_NC_1: control queue full, chnl=%u\n", (unsigned)chnlno);
                            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);
                            InformationObject_destroy(io);
                            IMasterConnection_sendASDU(connection, asdu);
                            return true;
                        }
                        /* 
                            基本各种工业协议都支持遥控、遥调 
                            而modbus 是最通用广泛协议，遥控遥调实现最初以modbus为原型
                            因此iec104的控制消息以modbus功能码来作为标识，不失通用性
                            对应的南向协议，如果是modbus协议，自动适配。如果是其他协议（如645协议）
                            会在南向动态库中做转换适配
                        
                        */
                        pctrl->funcode = _FC_WRITE_MULTIPLE_REGISTERS;
                        pctrl->is_active = 1;
                        pctrl->db_linkid = pNode_attr->db_link;
                        pctrl->value     = SetpointCommandShort_getValue(sc);
                    }

                    CS101_ASDU_setCOT(asdu, CS101_COT_ACTIVATION_CON);//设置激活确认
                    InformationObject_destroy(io);
                    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

                    if(pChnl->debug)
                        printf("IOA: %i switch to %f\n", ioa,
                            SetpointCommandShort_getValue(sc));//SCS，0代表分/开，1代表合/关

                    return true;
                }
                i++;
            }

            // can not found the ioa
            CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_IOA);
            InformationObject_destroy(io);
            IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

            return true;

        }
        else {
            printf("ERROR: message has no valid information object\n");
            return true;
        }
    }
    else
        CS101_ASDU_setCOT(asdu, CS101_COT_UNKNOWN_COT);

    IMasterConnection_sendASDU(connection, asdu);//发送激活确认信息

    return true;

}






/*********************************************************************************************************
** Function name:
** Descriptions: 除查询请求外，其他消息类型处理
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
bool iec104_slave_asduHandler(void* parameter, IMasterConnection connection, CS101_ASDU asdu)
{
//      直控的一般流程如下: 主站下发执行命令→装置回执行确认报文→完成结束报文。
//这里是装置回执行确认报文
    int TypeID;
    struct config_chnl_working_attr *pChnl = (struct config_chnl_working_attr *)parameter;

    if(pChnl->debug)
    {
        int id;
        id = CS101_ASDU_getTypeID(asdu);
        printf("asdu command handle:%d  %s\n", id, TypeID_toString(id));
    }

    TypeID =  CS101_ASDU_getTypeID(asdu);
    switch(TypeID)
    {
        case C_SC_NA_1:  //单命令判断
            return iec104_slave_asdu_C_SC_NA_1(pChnl, connection, asdu);
        break;
        case C_SE_NA_1:  //标度化设置  /* 48 - Set-point command, normalized value */
            return iec104_slave_asdu_C_SE_NA_1(pChnl, connection, asdu);
        break;
        case C_SE_NC_1:  //短浮点设值 /* 50 - Set-point command, short floating point number */
            return iec104_slave_asdu_C_SE_NC_1(pChnl, connection, asdu);
        break;

    }

    return false;
}

