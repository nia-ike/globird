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
#include "dlt645_97_datatype.h"
#include "dlt645_97_cfg.h"
#include "dlt645_97.h"

#include "pkt.h"

/*******************************************************************************************************************
.  DLT645包帧标准格式
·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符

·　DLT645读数据的包格式如下：
·　功能：请求读电能表数据
·　控制码：C=01H
·　数据域长度：L=02H+m（数据长度）
************************************************************************************************************************/

/****************************************************************************************************
    函数名称:       *Dlt645_R_callfunc
    功能描述:       回调函数实现DLT645读数据的功能，用户根据函数里的数据标识DataID，获取对应的数据项，输出到pOutData
	                缓存里；
    输入参数:  输入参数:     
	   <1> DLT645标准说明
		   DataID:
				1）为《DLT645-2007多功能电能表通信协议数据标识编码表》里的数据标识;
				2）即为包格式里的----DIODI1DI2DI3（数据标识）
				3）数据标识能标记唯一的标量(如：DataID:0x00000000;此数据标识表示当前组合有功费率电能
 		
		pInParam: 这取决于InLen的长度；
		         1）InLen=0时没有pInParam=NULL；
		         2）InLen=1时pInParam的数据长度为1；值为N（负荷记录块数)
				 6）InLen=6时pInParam的数据长度为6；pInParam的组织形式为 “N（负荷记录块数）+分|时|日|月|年”
		InLen  :   要刷新的数据的长度;对应写帧格式里：m(数据长度)
		
		pOutData：
		        1）为设备读取出来数据存放缓存区；pOutData的格式参考《DLT645-2007多功能电能表通信协议数据标识编码表》
		        2） 格式一般用BCD编码；
				
		OutLen ：为此缓存的大小；用此判断是否越界;
		
    输出参数:     
    返 回 值:    读出来的数据长度
    说    明:   输出的数据要满足《DLT645-2007多功能电能表通信协议数据标识编码表》要求；采用BCD碥码
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
    for(i=0; i<DLT645_97_COMM_ADDR_SIZE; i++)
    {		
		temp = ((0xff & (pAddr[i] >> 4) * 10) + (0xf & pAddr[i])) ;
		wAddr += (temp * pow(100,i));
    }   
    return wAddr;
}
/************************************************************************
函数功能：ProcessMsg91h	      11H解包函数，从站正常应答帧，91H无后续数据帧
输入参数：pFrameInfo	  数据帧管理结构
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg81h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint8_t *val)
{
	int 					iByteNum, i, nodeSize;
	uint16_t 				RegPos;
	double 					fVal;
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
	
	iByteNum = mb_encode_decode.UnPacket81H((uint8_t *)prequest_list->precv_buff, val);

	if(iByteNum > 0)
	{
		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
		if(!pIndexInfo)
			return -1;

		nodeSize = dlt645_97_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = dlt645_97_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;		  
			if(pNodeAttr->code != _FC_DLT645_97_READDATA)
				continue;

			/* 
				this node from the setting table is in this packet message.  
			*/
			if(pNodeAttr->reg_no == pRequestInfo->reg_no)
			{
				if(pNodeAttr->dataoffset)
					RegPos = pNodeAttr->dataoffset;
				else 
					RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no + pNodeAttr->dataoffset;

				DLT645V07_DatatoValue(&val[RegPos], pNodeAttr->datatype, &fVal);
				fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2); // +0.0005 四舍五入	

//				printf("ID:%08X RegPos:%d val[RegPos]:%d fVal:%lf, pNodeAttr->datatype:%d\n", pNodeAttr->reg_no, RegPos, val[RegPos], fVal, pNodeAttr->datatype);				
				
				/* 数据范围清洗使能  0-关闭清洗原样赋值   1-满足范围该点值为1，否则为0   2-满足该范围才更新值 */
				switch(pNodeAttr->range_checked)
				{
					case 1:
						if(((int)fVal >= pNodeAttr->range_min) && ((int)fVal <= pNodeAttr->range_max))
							fVal = 1;
						else 
							fVal = 0;

						DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
					break;
					case 2:
						if(((int)fVal >= pNodeAttr->range_min) && ((int)fVal <= pNodeAttr->range_max))
							DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
					break;
					default :
						DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
					break;
				}
			}

			

		}
	}
	
	return 0;
}



/************************************************************************
函数功能：ProcessMsg94h	     写数据94H 解包函数，从站正常应答帧
输入参数：pFrameInfo	  数据帧管理结构
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg84h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint8_t *val)
{
	int 					iByteNum, i, nodeSize;
	uint16_t 				RegPos;
	double 					fVal;
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
	
	iByteNum = mb_encode_decode.UnPacket84H((uint8_t *)prequest_list->precv_buff, val);

	if(iByteNum > 0)
	{
		/* respond byte must be larger(or eq) than request 8*RegNum */
		// if(iByteNum < (pFrameInfo->RegNum * 8))
		// 	return -1;

		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
		if(!pIndexInfo)
			return -1;

		nodeSize = dlt645_97_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = dlt645_97_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;
			
			/* 
				this node from the setting table is in this packet message.  
			*/
			if(pNodeAttr->reg_no == pRequestInfo->reg_no)
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;

				DLT645V07_DatatoValue(&val[RegPos],pNodeAttr->datatype,&fVal);
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
	LPDLT645HEAD		 	pHead;
	uint8_t					val[125];
	uint64_t				addr = 0;
    uint8_t                 LeadByteNum = 0;

    while((*(prequest_list->precv_buff + LeadByteNum) != _FC_DLT645_97_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }

	pHead	 = (LPDLT645HEAD)(prequest_list->precv_buff+LeadByteNum);
   
	addr = GetAddr(pHead->SlaveAddr);
	if(addr != prequest_list->phy_id)
	{
		return -1;
	}

	switch(pHead->CtlCode)
	{
		case _FC_DLT645_97_READDATA_ACK_FINISHED	:
			ret = ProcessMsg81h(pDevInfoAttr, prequest_list, val);
		break; 
		case _FC_DLT645_97_WRITEDATA_ACK_OK	:
			ret = ProcessMsg84h(pDevInfoAttr, prequest_list, val);
		break; 

		default:
			ret = -1;
		break;
	}

	 
	return ret;
	
}





