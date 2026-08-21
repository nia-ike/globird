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
#include "modbus-datatype.h"
#include "modbus-cfg.h"
#include "modbus-rtu.h"

#include "pkt.h"



/************************************************************************
函数功能：ProcessMsg01h	      01H解包函数
输入参数：pFrameInfo	  数据帧管理结构
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg01h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	int 					iByteNum, i, nodeSize;
	uint16_t 				RegPos;
	double 					fVal;
	struct config_node_attr *pNodeAttr;
	struct db_spl_remap_info_t *pIndexInfo;
	struct config_pkt_request_info  *pRequestInfo;
	struct config_devices_attr 		*pDevAttr;

	/* 本设备属性信息 */
	pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
	if(!pDevAttr)
		return -1;

	/* 帧信息 */
	pRequestInfo = (struct config_pkt_request_info *)prequest_list->private;
	
	iByteNum = mb_encode_decode.UnPacket01H((uint8_t *)prequest_list->precv_buff, val);
	// printf("iByteNum %d val %d reg %d\r\n",iByteNum, *val, pFrameInfo->RegNum);
	if(iByteNum > 0)
	{
		/* respond byte must be larger(or eq) than request 8*RegNum */
		// if(iByteNum < (pFrameInfo->RegNum * 8))
		// 	return -1;

		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
		if(!pIndexInfo)
			return -1;

		nodeSize = modbus_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = modbus_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;
			if(pNodeAttr->code != _FC_READ_COILS)
				continue;
			/* 
				this node from the setting table is in this packet message.  
			*/
			if((pNodeAttr->reg_no >= pRequestInfo->reg_no) && (pNodeAttr->reg_no < (pRequestInfo->reg_no + pRequestInfo->reg_num)))
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;
				/* 
					val[RegPos/8] >> (RegPos%8)
					notice: RegPos / 8    =>  RegPos >> 3
							RegPos % 8    =>  RegPos &  7
					fVal = (double)((val[RegPos / 8] >> (RegPos % 8)) & 1);		
				*/
				fVal = (double)((val[RegPos >> 3] >> (RegPos & 7)) & 1);
				DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);

			}
			
		}
	}
	
	return 0;
}

/************************************************************************
函数功能：ProcessMsg02h	      02H解包函数
输入参数：pFrameInfo	  数据帧管理结构
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg02h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	int 					iByteNum, i, nodeSize;
	uint16_t 				RegPos;
	double 					fVal;
	struct config_node_attr *pNodeAttr;
	struct db_spl_remap_info_t *pIndexInfo;
	struct config_pkt_request_info  *pRequestInfo;
	struct config_devices_attr 		*pDevAttr;

	/* 本设备属性信息 */
	pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
	if(!pDevAttr)
		return -1;

	/* 帧信息 */
	pRequestInfo = (struct config_pkt_request_info *)prequest_list->private;
	
	iByteNum = mb_encode_decode.UnPacket02H((uint8_t *)prequest_list->precv_buff, val);
	// printf("iByteNum %d val %d reg %d\r\n",iByteNum, *val, pFrameInfo->RegNum);
	if(iByteNum > 0)
	{
		/* respond byte must be larger(or eq) than request 8*RegNum */
		// if(iByteNum < (pFrameInfo->RegNum * 8))
		// 	return -1;

		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);
		if(!pIndexInfo)
			return -1;

		nodeSize = modbus_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = modbus_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;
			if(pNodeAttr->code != _FC_READ_DISCRETE_INPUTS)
				continue;
			/* 
				this node from the setting table is in this packet message.  
			*/
			if((pNodeAttr->reg_no >= pRequestInfo->reg_no) && (pNodeAttr->reg_no < (pRequestInfo->reg_no + pRequestInfo->reg_num)))
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;
				/* 
					val[RegPos/8] >> (RegPos%8)
					notice: RegPos / 8    =>  RegPos >> 3
							RegPos % 8    =>  RegPos &  7
					fVal = (double)((val[RegPos / 8] >> (RegPos % 8)) & 1);		
				*/
				// printf("RegPos = %d reg_no = %d\r\n", RegPos, pNodeAttr->reg_no);
				// printf("val %d\r\n", val[RegPos]);
				fVal = (double)((val[RegPos >> 3] >> (RegPos & 7)) & 1);
				DataMgmtFops.writepoint_in1dev(pIndexInfo, i, &fVal);
			}
			
		}
	}
	
	return 0;
}


/************************************************************************
函数功能：GenerateBitMask	      根据bitnum生成从最低位开始的连续1的掩码
输入参数：bitnum	 要设置的1的个数
输出参数：
返回值	：生成的掩码值
说明    ：例如 bitnum=3 时返回 0x7 (二进制: 0000000000000111)
************************************************************************/
static uint16_t GenerateBitMask(uint8_t bitnum)
{
    if (bitnum >= 16) {
        return 0xFFFF;  // 如果bitnum大于等于16，返回全1
    }
    return (1 << bitnum) - 1;  // 生成从最低位开始的连续bitnum个1
}


/************************************************************************
函数功能：ProcessMsg03h	      03H解包函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg03h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	int 							iRegNum, i, nodeSize;
	uint16_t 						RegPos;
	double 							fVal;
	struct config_node_attr 		*pNodeAttr;
	struct db_spl_remap_info_t 		*pIndexInfo;
	struct config_pkt_request_info  *pRequestInfo;
	struct config_devices_attr 		*pDevAttr;

	/* 本设备属性信息 */
	pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
	if(!pDevAttr)
		return -1;

	/* 帧信息 */
	pRequestInfo = (struct config_pkt_request_info *)prequest_list->private;
	iRegNum = mb_encode_decode.UnPacket03H((uint8_t *)prequest_list->precv_buff, val);	
	if(iRegNum > 0)
	{
		if(iRegNum != pRequestInfo->reg_num)
			return -1;
		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);	
		if(!pIndexInfo)
			return -1;
		nodeSize = modbus_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = modbus_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;
			if(pNodeAttr->code != _FC_READ_HOLDING_REGISTERS)
				continue;

			/* 
				this node from the setting table is in this packet message.  
			*/
			if((pNodeAttr->reg_no >= pRequestInfo->reg_no) && (pNodeAttr->reg_no < (pRequestInfo->reg_no+iRegNum)))
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;
				if(pNodeAttr->datatype == DATATYPE_DATA_BITS)
				{
					
					if(pNodeAttr->bitnum <= 1)
					{
						fVal = round((double)((val[RegPos] >> pNodeAttr->bitfield) & 0x01));
					}
					else 
					{
						fVal = round((double)((val[RegPos] >> pNodeAttr->bitfield) & GenerateBitMask(pNodeAttr->bitnum)));
						
					}
					
				}
				else
				{
					Modbus_RegDatatoDouble(pNodeAttr->datatype, pNodeAttr->bitfield, &val[RegPos], &fVal);
					fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2) + pNodeAttr->bfactor  ; // 
					/* 四舍五入 3为小数点 */
					fVal = round(fVal * 1000);
					fVal = fVal / 1000;
				}

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
		return 0;
	}
	
	return -1;
}


/************************************************************************
函数功能：ProcessMsg04h	      04H解包函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg04h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	int 							iRegNum, i, nodeSize;
	uint16_t 						RegPos;
	double 							fVal;
	struct config_node_attr 		*pNodeAttr;
	struct db_spl_remap_info_t 		*pIndexInfo;
	struct config_pkt_request_info  *pRequestInfo;
	struct config_devices_attr 		*pDevAttr;

	/* 本设备属性信息 */
	pDevAttr =  (struct config_devices_attr *)pDevInfoAttr->private;
	if(!pDevAttr)
		return -1;

	/* 帧信息 */
	pRequestInfo = (struct config_pkt_request_info *)prequest_list->private;
	
	iRegNum = mb_encode_decode.UnPacket04H((uint8_t *)prequest_list->precv_buff, val);	
	if(iRegNum > 0)
	{
		if(iRegNum != pRequestInfo->reg_num)
			return -1;

		pIndexInfo = DataMgmtFops.getIndexByPortAndDev(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id);	
		if(!pIndexInfo)
			return -1;

		nodeSize = modbus_cfg_node_attr_size(pDevAttr);
		for(i=0; i<nodeSize; i++)
		{
			pNodeAttr = modbus_cfg_node_attr(pDevAttr, i);
			if(pNodeAttr == 0)
				continue;

			if(pNodeAttr->code != _FC_READ_INPUT_REGISTERS)
				continue;

//printf("nodeSize:%d  i:%d  %p  => %d:   \n", nodeSize, i, pNodeAttr, pNodeAttr->reg_no);
			/* 
				this node from the setting table is in this packet message.  
			*/
			if((pNodeAttr->reg_no >= pRequestInfo->reg_no) && (pNodeAttr->reg_no < (pRequestInfo->reg_no+iRegNum)))
			{
				RegPos = pNodeAttr->reg_no - pRequestInfo->reg_no;
				if(pNodeAttr->datatype == DATATYPE_DATA_BITS)
				{
					if(pNodeAttr->bitnum <= 1)
					{
						fVal = round((double)((val[RegPos] >> pNodeAttr->bitfield) & 0x01));
					}
					else 
					{
						fVal = round((double)((val[RegPos] >> pNodeAttr->bitfield) & GenerateBitMask(pNodeAttr->bitnum)));
					}
				}
				else
				{
					Modbus_RegDatatoDouble(pNodeAttr->datatype, pNodeAttr->bitfield, &val[RegPos], &fVal);
					fVal = fVal * (pNodeAttr->kfactor * pNodeAttr->pt_flag1 * pNodeAttr->ct_flag2) + pNodeAttr->bfactor ; // +0.0005 四舍五入
					/* 四舍五入 3为小数点 */
					fVal = round(fVal * 1000);
					fVal = fVal / 1000;
				}
				
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
		return 0;
	}
	
	return -1;
}


/************************************************************************
ProcessMsg05h	      05H解包函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg05h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	return mb_encode_decode.UnPacket05H((uint8_t *)prequest_list->precv_buff);
}


/************************************************************************
ProcessMsg05h	      05H解包函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg06h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	return mb_encode_decode.UnPacket06H((uint8_t *)prequest_list->precv_buff);
}

/************************************************************************
ProcessMsg05h	      05H解包函数
输入参数：pBuf	  数据帧首地址
		   nSize	 数据帧长度
		   pVoid	 额外参数  （根据协议不同传入）
输出参数：
返回值	：帧 list 指针
说明    ：
************************************************************************/
static int32_t ProcessMsg10h(struct config_devices_info_attr *pDevInfoAttr, struct config_request_list *prequest_list, uint16_t *val)
{
	return mb_encode_decode.UnPacket10H((uint8_t *)prequest_list->precv_buff);
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
	LPModBusHEAD		 	pHead;
	uint16_t				val[125];

	pHead	 = (LPModBusHEAD)prequest_list->precv_buff;
	if(pHead->SlaveAddr != prequest_list->phy_id)
	{
		return -1;
	}

	switch(pHead->Cmd)
	{
		case _FC_READ_COILS	:
			ret = ProcessMsg01h(pDevInfoAttr, prequest_list, val);
		break; 
		case _FC_READ_DISCRETE_INPUTS	:
			ret = ProcessMsg02h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_READ_HOLDING_REGISTERS	:
			ret = ProcessMsg03h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_READ_INPUT_REGISTERS	:
			ret = ProcessMsg04h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_WRITE_SINGLE_COIL	:
			ret = ProcessMsg05h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_WRITE_SINGLE_REGISTER	:
			ret = ProcessMsg06h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_READ_EXCEPTION_STATUS	:
		
		break;
		case _FC_WRITE_MULTIPLE_COILS	:

		break;
		case _FC_WRITE_MULTIPLE_REGISTERS	:
			ret = ProcessMsg10h(pDevInfoAttr, prequest_list, val);
		break;
		case _FC_REPORT_SLAVE_ID	:

		break;
		case _FC_WRITE_AND_READ_REGISTERS	:

		break;
		default:
			ret = -1;
		break;
	}

	 
	return ret;
	
}





