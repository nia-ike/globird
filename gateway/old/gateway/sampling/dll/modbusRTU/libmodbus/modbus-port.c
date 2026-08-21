/*
 * File      : config_devices.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */


#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include "modbus-cfg.h"
#include "modbus-rtu.h"
#include "modbus-process.h"
#include "pkt.h"
#include "db_node_fops.h"
#include "modbus-datatype.h"

/*
    通讯协议驱动使用.so 动态库形式动态加载。 
    每个通讯协议驱动按理只需要port.c即可。只要对外 实现 _create  _destroy  2个函数

    对内实现  modbus_pkt_request_list_create，用于生成和装载发送帧

    如果 modbus_pkt_request_list_create 发送帧有多种实现方式，比如动态和静态：

    1. 如本示例，通过寄存器号和长度生成发送帧
    2. 静态报文。这时候只需要硬编码生成多个数组，调用pkt_request_list_static_append即可，如
       pkt_group [][] = {
           {01, 03, 00 ,00 ,00 ,02 , CRCL, CRCH},
           {01, 03, 00 ,02 ,00 ,02 , CRCL, CRCH},
           {01, 03, 00 ,04 ,00 ,02 , CRCL, CRCH},
           ...
       }
       静态通常用于  只有单台节点，无地址，内容固定的设备。如某ups设备，只发固定字符串
       静态代码实现非常简单


       注意： 本驱动框架不能有 全局变量 和静态变量。  所有变量需要动态申请，比如 malloc， 请参考lib_create
*/



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本设备请求帧装载
                         所有问答式的协议驱动，均需要调用pkt.c 中的接口，把协议生成的发送帧，append到发送链表队列中pkt_request_list_static_append
                         每调用一次pkt_request_list_static_append，即在本设备中增加发送帧链表节点
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
static void modbus_pkt_request_list_create(struct config_devices_info_attr *pDevInfoAttr, struct config_devices_attr *pDevAttr)
{
    uint8_t	npkt;
    int i;
    uint8_t buff[256];
    int nsend;

    /* do not larger than CONFIG_MAX_REQUEST_PKT_LEN */
    npkt = (pDevAttr->pkg_request.pkt_num > CONFIG_MAX_REQUEST_PKT_LEN) ? CONFIG_MAX_REQUEST_PKT_LEN : pDevAttr->pkg_request.pkt_num;

    for(i=0; i<npkt; i++)
    {
        switch (pDevAttr->pkg_request.pkt_req_info[i].code)
        {
        case _FC_READ_COILS:
            nsend = mb_encode_decode.Packet01H(buff, pDevInfoAttr->phy_id, pDevAttr->pkg_request.pkt_req_info[i].reg_no, pDevAttr->pkg_request.pkt_req_info[i].reg_num);
            if(nsend > 0)
                pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->pkg_request.pkt_req_info[i]);
            break;
        
        case _FC_READ_DISCRETE_INPUTS:
            nsend = mb_encode_decode.Packet02H(buff, pDevInfoAttr->phy_id, pDevAttr->pkg_request.pkt_req_info[i].reg_no, pDevAttr->pkg_request.pkt_req_info[i].reg_num);
            if(nsend > 0)
                pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->pkg_request.pkt_req_info[i]);
            break;
        
        case _FC_READ_HOLDING_REGISTERS:
            nsend = mb_encode_decode.Packet03H(buff, pDevInfoAttr->phy_id, pDevAttr->pkg_request.pkt_req_info[i].reg_no, pDevAttr->pkg_request.pkt_req_info[i].reg_num);
            if(nsend > 0)
                pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->pkg_request.pkt_req_info[i]);				
            break;
        
        case _FC_READ_INPUT_REGISTERS:
            nsend = mb_encode_decode.Packet04H(buff, pDevInfoAttr->phy_id, pDevAttr->pkg_request.pkt_req_info[i].reg_no, pDevAttr->pkg_request.pkt_req_info[i].reg_num);
            if(nsend > 0)
                pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->pkg_request.pkt_req_info[i]);
            break;
        
        default:
            break;
        }
    }
}





/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本驱动初始化
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int32_t lib_create(struct config_devices_info_attr *pDevInfoAttr)
{
    struct config_devices_attr *pDevAttr;

    /*  0. 实时数据库绑定 */
    DataMgmtFops.init(pDevInfoAttr->pVmem);

    /*
        1. 申请当前帧点表属性管理空间
    */
    pDevAttr = calloc(1, sizeof(struct config_devices_attr));
    if(!pDevAttr)
        return -1;

    /*
        2. 保存当前点表到设备信息管理中
    */
    pDevInfoAttr->private = pDevAttr;

    pDevInfoAttr->pfnFlowCallback = NULL;

    /*
        3. 从json加载当前设备的配置
    */
    modbus_cfg_load(pDevAttr, pDevInfoAttr->cfg_file);

    /*
        4. 根据配置生成发送帧，并压入本设备发送帧队列
           由于每个协议可能存在多个设备，这里通过动态申请内存，实现多个设备公用一个库
    */
    modbus_pkt_request_list_create(pDevInfoAttr, pDevAttr);

    return 0;
}



/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 本驱动初始化
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
void lib_destroy(struct config_devices_info_attr *pDevInfoAttr)
{
    /* 释放请求报文申请的内存 */
    pkt_request_list_all_destory(pDevInfoAttr);

    /* 释放点表申请的内存 */
    if(pDevInfoAttr->private)
        free(pDevInfoAttr->private);

}




/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
int32_t lib_control(struct config_devices_info_attr *pDevInfoAttr, void *pargs, void *pbuffer, uint32_t len)
{
    int nodeSize, i;
    struct db_spl_channel_ctr_t     *pctrl = (struct db_spl_channel_ctr_t *)pargs;
    struct config_devices_attr      *pDevAttr;
    struct config_node_attr 		*pNodeAttr;
    struct config_control_attr 		*pNodeCtrlAttr;
    
    int n, j;
    int16_t value;
    double  lfval;
    int16_t  iarry_value[16];
    uint16_t uarry_value[16];
    /* 本设备属性信息 */
    pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
    if(!pDevAttr)
        return -1;

    /* 
        支持2种控制：
        1. 从共享内存的pctrl中制定的控制方式
        2. 从南向的配置 control_attr 设定的控制方式

        --------------------------------
        其中：
        1. 用于确切的外部需求控制（比如北向modbusTCP，IEC104等，确切需要标准方式  遥信遥测 对应 遥控遥调功能码
        2. 用于南向时，设备的遥控遥调非标准（不与遥信遥测匹配）时，根据配置工作（判断优先级最高）


    
    */

    
    /*
        方式二：  优先级最高。客户有指定配置，即以手动配置工作
    */
    if((DB_CONVENT_ID_2_TYPE(pctrl->db_linkid) == 0x400) || (DB_CONVENT_ID_2_TYPE(pctrl->db_linkid) == 0x300))
    {
        nodeSize = modbus_cfg_nodectrl_attr_size(pDevAttr);
        for(i=0; i<nodeSize; i++)
        {
            pNodeCtrlAttr = modbus_cfg_nodectrl_attr(pDevAttr, i);
            if(pNodeCtrlAttr == NULL)
                continue;
            /* 
                this node from the setting table is in this packet message.  
            */
            if(pNodeCtrlAttr->id == DB_CONVENT_ID_2_INDEX(pctrl->db_linkid))
            {
                /*  */	
                switch(pNodeCtrlAttr->funcode)
                {
                    case _FC_WRITE_SINGLE_REGISTER:
                        if(DB_CONVENT_ID_2_TYPE(pctrl->db_linkid) != 0x400)
                        {
                            value = ((pctrl->value  / pNodeCtrlAttr->kfactor)) ;
                            if(value == 0)
                            {
                                return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno0,  pNodeCtrlAttr->regvalue0, 0, 0);
                            }
                            else 
                            {
                                return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno1,  pNodeCtrlAttr->regvalue1, 0, 0);
                            }
                        }
                        else 
                        {
                            // 0x400  为遥调的控制点。执行客户写入的数据
                            lfval = ((pctrl->value  / pNodeCtrlAttr->kfactor )) ;

                            /* 获取当前控制点属性中，从数据类型获取占用的寄存器号 */
                            n = Modbus_DataTypeToRegNum(pNodeCtrlAttr->datatype);
                            if(n < 0)
                                return -1;

                            if(lfval > 0)
                            {
                                // 根据数据类型，转换 成寄存器值
                                for(j=0; j<n; j++)
                                {
                                    Modbus_DoubleToReg16_CtrlCmd(pNodeCtrlAttr->datatype, j, (uint16_t *)&uarry_value[j], &lfval);

                                }

                                switch (pNodeCtrlAttr->funcode)
                                {
                                    case _FC_WRITE_SINGLE_REGISTER:
                                        return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  uarry_value[0], 0, 0);

                                    case _FC_WRITE_MULTIPLE_REGISTERS:
                                        return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  n, (uint16_t *)uarry_value, 0, 0);

                                    default:
                                        break;
                                }
                            }
                            else 
                            {
                                // 根据数据类型，转换 成寄存器值
                                for(j=0; j<n; j++)
                                {
                                    Modbus_DoubleToReg16_CtrlCmd(pNodeCtrlAttr->datatype, j, (uint16_t *)&iarry_value[j], &lfval);

                                }

                                switch (pNodeCtrlAttr->funcode)
                                {
                                    case _FC_WRITE_SINGLE_REGISTER:
                                        return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  iarry_value[0], 0, 0);

                                    case _FC_WRITE_MULTIPLE_REGISTERS:
                                        return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  n, (uint16_t *)iarry_value, 0, 0);

                                    default:
                                        break;
                                }
                            }
                        }

                    case _FC_WRITE_MULTIPLE_REGISTERS:
                        if(DB_CONVENT_ID_2_TYPE(pctrl->db_linkid) != 0x400)
                        {
                            value = ((pctrl->value  / pNodeCtrlAttr->kfactor)) ;
                            if(value == 0)
                            {
                                return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno0,  1, &pNodeCtrlAttr->regvalue0, 0, 0);
                            }
                            else 
                            {
                                return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno1,  1, &pNodeCtrlAttr->regvalue1, 0, 0);
                            }
                        }
                        else 
                        {
                            // 0x400  为遥调的控制点。执行客户写入的数据
                            lfval = ((pctrl->value  / pNodeCtrlAttr->kfactor )) ;

                            /* 获取当前控制点属性中，从数据类型获取占用的寄存器号 */
                            n = Modbus_DataTypeToRegNum(pNodeCtrlAttr->datatype);
                            if(n < 0)
                                return -1;

                            if(lfval > 0)
                            {
                                // 根据数据类型，转换 成寄存器值
                                for(j=0; j<n; j++)
                                {
                                    Modbus_DoubleToReg16_CtrlCmd(pNodeCtrlAttr->datatype, j, (uint16_t *)&uarry_value[j], &lfval);

                                    // 沿用原来的数据处理接口，这里需要先交换
                                    if(n < 2)
                                    {
                                        uarry_value[j] = SWAP_16BIT(uarry_value[j]);
                                    }
                                }

                                switch (pNodeCtrlAttr->funcode)
                                {
                                    case _FC_WRITE_SINGLE_REGISTER:
                                        return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  uarry_value[0], 0, 0);

                                    case _FC_WRITE_MULTIPLE_REGISTERS:
                                        return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  n, (uint16_t *)uarry_value, 0, 0);

                                    default:
                                        break;
                                }
                            }
                            else 
                            {
                                // 根据数据类型，转换 成寄存器值
                                for(j=0; j<n; j++)
                                {
                                    Modbus_DoubleToReg16_CtrlCmd(pNodeCtrlAttr->datatype, j, (uint16_t *)&iarry_value[j], &lfval);

                                    // 沿用原来的数据处理接口，这里需要先交换
                                    if(n < 2)
                                    {
                                        iarry_value[j] = SWAP_16BIT(iarry_value[j]);
                                    }
                                }

                                switch (pNodeCtrlAttr->funcode)
                                {
                                    case _FC_WRITE_SINGLE_REGISTER:
                                        return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  iarry_value[0], 0, 0);

                                    case _FC_WRITE_MULTIPLE_REGISTERS:
                                        return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno,  n, (uint16_t *)iarry_value, 0, 0);

                                    default:
                                        break;
                                }
                            }	
                            
                        }
                    
                    case _FC_WRITE_SINGLE_COIL:
                        value = ((pctrl->value  / pNodeCtrlAttr->kfactor )) ;
                        if(value == 0)
                        {
                            return mb_encode_decode.Packet05H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno0, pNodeCtrlAttr->regvalue0);
                        }
                        else 
                        {
                            return mb_encode_decode.Packet05H(pbuffer, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno1, pNodeCtrlAttr->regvalue1);
                        }
                }
            }
        }
    }

    /* 
        方式一：
     */
    nodeSize = modbus_cfg_node_attr_size(pDevAttr);
    double fVal;
    for(i=0; i<nodeSize; i++)
    {
        pNodeAttr = modbus_cfg_node_attr(pDevAttr, i);
        if(pNodeAttr == NULL)
            continue;
        
        /* 
            this node from the setting table is in this packet message.  
        */
        if(pNodeAttr->id == DB_CONVENT_ID_2_INDEX(pctrl->db_linkid))
        {
            switch(pctrl->funcode)
            {
                case _FC_WRITE_SINGLE_REGISTER:
                    value = ((pctrl->value  / pNodeAttr->kfactor ));
                    fVal  = ((pctrl->value  / pNodeAttr->kfactor ));
                    //  串口通道1-8.   网络通道17-64.   其中9-16 用于空串口用途，建立虚拟设备，用于多个主站通过改虚拟设备 进行主站之间数据交互（仅modbusRTU）
                    if((pDevInfoAttr->channel_id >= 9) && (pDevInfoAttr->channel_id <= 16)) 
                    {
                        // 虚拟设备仅支持写入内部存储，不存在写modbus报文
                        DataMgmtFops.writepoint_by_id(pctrl->db_linkid, &fVal); 
                    }
                    else 
                    {
                        return mb_encode_decode.Packet06H(pbuffer, pDevInfoAttr->phy_id, pNodeAttr->reg_no,  value, 0, 0);
                    }

                case _FC_WRITE_MULTIPLE_REGISTERS:
                    fVal  = ((pctrl->value  / pNodeAttr->kfactor));
                    //  串口通道1-8.   网络通道17-64.   其中9-16 用于空串口用途，建立虚拟设备，用于多个主站通过改虚拟设备 进行主站之间数据交互（仅modbusRTU）
                    if((pDevInfoAttr->channel_id >= 9) && (pDevInfoAttr->channel_id <= 16))
                    {
                        // 虚拟设备仅支持写入内部存储，不存在写modbus报文
                        DataMgmtFops.writepoint_by_id(pctrl->db_linkid, &fVal);
                    }
                    else 
                    {
                        n = Modbus_DataTypeToRegNum(pNodeAttr->datatype);
                        if(n < 0)
                            return -1;

                        if (fVal > 0)
                        {
                            for (j=0; j<n; j++)
                            {
                                Modbus_DoubleToReg16_CtrlCmd(pNodeAttr->datatype, j, (uint16_t *)&uarry_value[j], &fVal);
                                if(n < 2)
                                {
                                    uarry_value[j] = SWAP_16BIT(uarry_value[j]);
                                }
                            }
                            return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeAttr->reg_no, n, (uint16_t *)&uarry_value, 0, 0);
                        }
                        else
                        {
                            for (j=0; j<n; j++)
                            {
                                Modbus_DoubleToReg16_CtrlCmd(pNodeAttr->datatype, j, (uint16_t *)&iarry_value[j], &fVal);
                                if(n < 2)
                                {
                                    iarry_value[j] = SWAP_16BIT(iarry_value[j]);
                                }
                            }
                            return mb_encode_decode.Packet10H(pbuffer, pDevInfoAttr->phy_id, pNodeAttr->reg_no, n, (uint16_t *)&iarry_value, 0, 0);
                        }
                    }
                
                case _FC_WRITE_SINGLE_COIL:
                    //  串口通道1-8.   网络通道17-64.   其中9-16 用于空串口用途，建立虚拟设备，用于多个主站通过改虚拟设备 进行主站之间数据交互（仅modbusRTU）
                    if((pDevInfoAttr->channel_id >= 9) && (pDevInfoAttr->channel_id <= 16))
                    {
                        fVal = pctrl->value;
                        // 虚拟设备仅支持写入内部存储，不存在写modbus报文
                        DataMgmtFops.writepoint_by_id(pctrl->db_linkid, &fVal);
                    }
                    else 
                    {
                        return mb_encode_decode.Packet05H(pbuffer, pDevInfoAttr->phy_id, pNodeAttr->reg_no, (uint16_t)pctrl->value);
                    }
            }

        }

    }

    return -1;
}