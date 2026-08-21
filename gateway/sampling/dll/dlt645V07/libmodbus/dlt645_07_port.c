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

#include "dlt645_07_cfg.h"
#include "dlt645_07.h"
#include "dlt645_07_process.h"
#include "pkt.h"
#include "db_node_fops.h"


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
static void dlt645_07_pkt_request_list_create(struct config_devices_info_attr *pDevInfoAttr, struct config_devices_attr *pDevAttr)
{
	uint8_t	npkt;
	int i;
	uint8_t buff[256];
	int nsend;

	/* do not larger than CONFIG_MAX_REQUEST_PKT_LEN */
	npkt = (pDevAttr->node_num > CFG_MAX_POINT) ? CFG_MAX_POINT : pDevAttr->node_num;
	for(i=0; i<npkt; i++)
	{
		switch (pDevAttr->node_attr[i].code)
		{
			case _FC_DLT645_07_READ_DATA:
				nsend = mb_encode_decode.Packet11H(buff, pDevAttr->node_attr[i].leadnum, pDevInfoAttr->phy_id, pDevAttr->node_attr[i].reg_no);
				if(nsend > 0)
					pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->node_attr[i]);
				break;
			case _FC_DLT645_07_READ_COMMADDR:
				nsend = mb_encode_decode.Packet13H(buff, pDevAttr->node_attr[i].leadnum, pDevInfoAttr->phy_id);
				if(nsend > 0)
					pkt_request_list_static_append(pDevInfoAttr, buff, nsend, fnReceiveCallback, NULL, &pDevAttr->node_attr[i]);
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
	dlt645_07_cfg_load(pDevAttr, pDevInfoAttr->cfg_file);



	/*
		4. 根据配置生成发送帧，并压入本设备发送帧队列
           由于每个协议可能存在多个设备，这里通过动态申请内存，实现多个设备公用一个库
	*/
	dlt645_07_pkt_request_list_create(pDevInfoAttr, pDevAttr);

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
	struct config_control_attr 		*pNodeCtrlAttr;
	
	uint16_t value;
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
	nodeSize = dlt645_07_cfg_nodectrl_attr_size(pDevAttr);
    for(i=0; i<nodeSize; i++)
    {
		pNodeCtrlAttr = dlt645_07_cfg_nodectrl_attr(pDevAttr, i);
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
				case _FC_DLT645_07_WRITE_DATA:
					value = (uint16_t)(pctrl->value + 0.5) ;
					if(value == 0)
					{
						return mb_encode_decode.Packet14H(pbuffer, pNodeCtrlAttr->leadbytenum, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno0, pNodeCtrlAttr->password,  pNodeCtrlAttr->operator,  &pNodeCtrlAttr->regvalue0, 2);			
					}	
					else
					{
						return mb_encode_decode.Packet14H(pbuffer, pNodeCtrlAttr->leadbytenum, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno1, pNodeCtrlAttr->password,  pNodeCtrlAttr->operator,  &pNodeCtrlAttr->regvalue1, 2);			
					}
				break;
				case _FC_DLT645_07_WRITE_CONTROL:
					value = (uint16_t)(pctrl->value + 0.5) ;
					if(value == 0)
					{
						return mb_encode_decode.Packet1CH(pbuffer, pNodeCtrlAttr->leadbytenum, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno0, pNodeCtrlAttr->password,  pNodeCtrlAttr->operator,  &pNodeCtrlAttr->regvalue0, 2);			
					}	
					else
					{
						return mb_encode_decode.Packet1CH(pbuffer, pNodeCtrlAttr->leadbytenum, pDevInfoAttr->phy_id, pNodeCtrlAttr->regno1, pNodeCtrlAttr->password,  pNodeCtrlAttr->operator,  &pNodeCtrlAttr->regvalue1, 2);			
					}
				break;
			}
		}
	}	
    return -1;
}


