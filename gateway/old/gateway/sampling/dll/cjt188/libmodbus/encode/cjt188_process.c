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

#include "debug.h"
#include "sampling_cfg.h"
#include "db_node_fops.h"
#include "cjt188_datatype.h"
#include "cjt188_cfg.h"
#include "cjt188.h"

#include "pkt.h"

/*******************************************************************************************************************
.  CJT188包帧标准格式
·　帧格式：| 帧起始符 | 仪表类型 | 地址域 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符 |
·　CJT188读数据的包格式如下：
·　功能：请求读计量数据
·　控制码：C=01H
·　数据域长度：L=03H（数据长度）
************************************************************************************************************************/

/****************************************************************************************************
    函数名称:       *Cjt188_R_callfunc
    功能描述:       回调函数实现CJT188读数据的功能，用户根据函数里的数据标识DataID，获取对应的数据项，输出到pOutData
	                缓存里；
    输入参数:  输入参数:     
	   <1> CJT188标准说明
		   DataID:
				1）为《CJT188户用计量仪表数据传输技术条件》里的数据标识;
				2）即为包格式里的----DIODI1DI2DI3（数据标识）
				3）数据标识能标记唯一的标量(如：DataID:0x901F;此数据标识表示读计量数据，根据仪表类型T来定义
 		
			T: 仪表类型代码；		         
				 0x10  //冷水水表
				 0x11  //生活热水水表
				 0x12  //直饮水水表
				 0x13  //中水水表
				 0x20  //热量表(计热量)
				 0x21  //热量表(计冷量)
				 0x30  //燃气表
				 0x40  //其他仪表，如电度表
		InLen  :   要刷新的数据的长度;对应写帧格式里：m(数据长度)		
		pOutData：
		        1）为设备读取出来数据存放缓存区；pOutData的格式参考《CJT188户用计量仪表数据传输技术条件》
		        2） 格式一般用BCD编码；
				
		OutLen ：为此缓存的大小；用此判断是否越界;
		
    输出参数:     
    返 回 值:    读出来的数据长度
    说    明:   输出的数据要满足《CJT188户用计量仪表数据传输技术条件》要求；采用BCD碥码
*******************************************************************************************************/

/************************************************************************
函数功能：      SetAddr()
功能描述:       地址域
输入参数:        
输出参数:       
返 回 值:      
说    明:       
************************************************************************/
uint64_t GetAddr(uint8_t *pAddr)
{
    uint8_t i;
    uint64_t temp = 0;
	uint64_t wAddr = 0;
    for(i=0; i<CJT188_COMM_ADDR_SIZE; i++)
    {
		temp = ((0xff & (pAddr[i] >> 4) * 10) + (0xf & pAddr[i])) ;
		wAddr += (temp * pow(100,i));
    }   
    return wAddr;
}
/************************************************************************
函数功能：ProcessMsg01h	      01H解包函数，从站正常应答帧
输入参数：pFrameInfo	  数据帧管理结构
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg01h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint8_t *val)
{
	int 					iByteNum, i, nodeSize;
	uint16_t 				RegPos;
	double 					fVal;
	uint8_t 				function;
	struct config_node_attr *pNodeAttr;
	struct db_spl_remap_info_t *pIndexInfo;
	struct config_node_attr  *pRequestInfo;
	struct config_devices_attr 		*pDevAttr;

	/* 本设备属性信息 */
	pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
	if(!pDevAttr)
		return -1;
	/* 帧信息 */
	pRequestInfo = (struct config_node_attr *)prequest_list->private;


	iByteNum = mb_encode_decode.UnPacket01H((uint8_t *)prequest_list->precv_buff, val);
	if(iByteNum > 0)
	{
		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
		if(!pIndexInfo)
			return -1;

		nodeSize = cjt188_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = cjt188_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;
			if(pNodeAttr->code != _FC_CJT188_READ_DATA)
				continue;
			/* 
				this node from the setting table is in this packet message.  
			*/
			if(pNodeAttr->reg_no == pRequestInfo->reg_no)
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;
				function = atoi(pDevAttr->function);
				CJT188BCDDatatoValue(&val[RegPos],pNodeAttr->datatype, function, pNodeAttr->bitfield,&fVal);
				fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2) + 0.0005; // +0.0005 四舍五入					
				DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
			}			
		}
	}	
	return 0;
}



/************************************************************************
函数功能：fnReceiveCallback	 解包回调函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
int32_t fnReceiveCallback(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list)
{
	int 					ret = -1;
	LPCJT188HEAD		 	pHead;
	uint8_t					val[125];
	uint64_t				addr = 0;
	uint8_t					LeadByteNum = 0;

	while((*(prequest_list->precv_buff + LeadByteNum) != _FC_CJT188_START_FLAG) && (LeadByteNum < 200))
	{
		LeadByteNum ++;
	}

	pHead	 = (LPCJT188HEAD)(prequest_list->precv_buff + LeadByteNum);
 
	addr = GetAddr(pHead->Addr);

	if(addr != prequest_list->phy_id)
	{
		return -1;
	}
	switch(pHead->CtlCode)
	{
		case _FC_CJT188_READ_DATA_ACK_OK	:
			ret = ProcessMsg01h(pDevInfoAttr, prequest_list, val);
		break; 
		default:
			ret = -1;
		break;
	}	 
	return ret;	
}





