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
#include "iec101-recv.h"
#include "iec101-utils.h"

#include "pkt.h"

static void Explain_M_SP_NA_1(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_DP_NA_3(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_ME_NA_9(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_ME_NB_11(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_ME_NC_13(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_IT_NA_15(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_ME_ND_21(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_M_SP_TB_30(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_C_SC_NA_45(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static void Explain_C_DC_NA_46(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen);
static bool FrameCheckout(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int32_t nLen, enum eRecvFrameType *pFrameType, struct S_CTRL *pCtrl);
static int RecvFrame(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nMaxLen, enum eRecvFrameType *pFrameType, struct S_CTRL *pCtrl);
static void SubmitRtValue(struct config_devices_attr *pDevAttr, int infAddr, double value);
static void SubmitDevCommStatus(struct config_devices_attr *pDevAttr, enum eCommStatus commStatus);
static struct config_control_attr *FindControlPt(struct config_devices_attr *pDevAttr, int infAddr);
static void ProcessCommFailed();
static void ProcessCommSucceed();

bool RecvAndExplainFrame(struct config_devices_attr *pDevAttr, struct S_CTRL *pCtrl)
{// 接收并解析数据，返回接收报文是否正确
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	uint8_t recvBuff[512] = { 0 };
	enum eRecvFrameType frameType = eRecvFrameType_NULL;
	int recvBytes = RecvFrame(pDevAttr, recvBuff, sizeof(recvBuff), &frameType, pCtrl);
	if (recvBytes <= 0 || frameType == eRecvFrameType_NULL)
	{// 接收报文错误
		ProcessCommFailed(pDevAttr);
		return false;
	}

	// 设备通讯正常
	ProcessCommSucceed(pDevAttr);

	if (frameType == eRecvFrameType_Fixed_Length)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到正确的回复\n", pContext->phy_id);
		return true;// 固定长度帧不需要做数据解析
	}

	uint8_t *pAsdu = &recvBuff[6+pDevAttr->addr_size-1];
	uint8_t asduLen = recvBuff[1]-2-(pDevAttr->addr_size-1);// 27
	switch (pAsdu[0])
	{// ASDU 类型标识
		case M_SP_NA_1:	//单点信息
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到单点信息\n", pContext->phy_id);
			Explain_M_SP_NA_1(pDevAttr, pAsdu, asduLen);
			break;

		case M_DP_NA_3:	//双点信息
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到双点信息\n", pContext->phy_id);
			Explain_M_DP_NA_3(pDevAttr, pAsdu, asduLen);
			break;

		case M_ME_NA_1:	//测量值，归一化值
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到归一化值\n", pContext->phy_id);
			Explain_M_ME_NA_9(pDevAttr, pAsdu, asduLen);
			break;

		case M_ME_NB_1:	//测量值，标度化值
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到标度化值\n", pContext->phy_id);
			Explain_M_ME_NB_11(pDevAttr, pAsdu, asduLen);
			break;

		case M_ME_NC_1:	//测量值，短浮点数
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到短浮点数\n", pContext->phy_id);
			Explain_M_ME_NC_13(pDevAttr, pAsdu, asduLen);
			break;

		case M_IT_NA_1:	//累计量
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到累计量\n", pContext->phy_id);
			Explain_M_IT_NA_15(pDevAttr, pAsdu, asduLen);
			break;

		case M_ME_ND_1:	//测量值，不带品质描述的归一化值
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到不带品质描述的归一化值\n", pContext->phy_id);
			Explain_M_ME_ND_21(pDevAttr, pAsdu, asduLen);
			break;

		case M_SP_TB_1: //带时标CP56Time2a的单点信息
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到带时标CP56Time2a的单点信息\n", pContext->phy_id);
			Explain_M_SP_TB_30(pDevAttr, pAsdu, asduLen);
			break;

		case C_SC_NA_1:	//单点命令
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到单点命令\n", pContext->phy_id);
			Explain_C_SC_NA_45(pDevAttr, pAsdu, asduLen);
			break;

		case C_DC_NA_1:	//双点命令
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到双点命令\n", pContext->phy_id);
			Explain_C_DC_NA_46(pDevAttr, pAsdu, asduLen);
			break;

		case C_IC_NA_1: //总召确认
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到总召确认或结束\n", pContext->phy_id);
			break;

		case C_CI_NA_1: //电能脉冲召唤命令
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到电能脉冲总召确认或结束\n", pContext->phy_id);
			break;

		default:
			if (pDevAttr->debug_enabled) printf("Rx设备%d: 收到未知的ASDU类型：%d\n", pContext->phy_id, pAsdu[0]);
			break;
	}
	
	return true;
}

static bool FrameCheckout(
	struct config_devices_attr *pDevAttr,
	uint8_t *pBuff,
	int32_t nLen,
	enum eRecvFrameType *pFrameType,
	struct S_CTRL *pCtrl)
{// 报文格式校验
	*pFrameType = eRecvFrameType_NULL;
	if (pBuff == NULL || nLen < 1) return false;
	
	uint8_t byFrameHead = pBuff[0];
	if (0x68 == byFrameHead)
	{// 可变长度帧
		if (nLen < 4) return false;
		if (pBuff[0] != 0x68 || pBuff[3] != 0x68) return false;
		
		uint8_t len = pBuff[1];
		if (nLen < len + 6) return false;
		
		uint8_t sumvalue = CheckSum8(&pBuff[4], len);
		if (pBuff[len + 4] != sumvalue ||
			pBuff[len + 5] != 0x16) return false;
		
		memcpy(pCtrl, &pBuff[4], 1);
		*pFrameType = eRecvFrameType_Variable_Length;
		return true;
	}
	else if (0x10 == byFrameHead)
	{// 固定长度帧
		if (nLen < 5) return false;
		
		if (pBuff[0] != 0x10) return false;
		if (pBuff[2] != pDevAttr->context_attr.phy_id) return false;
        if (pDevAttr->addr_size == 2)
        {
		    if (nLen < 6) return false;
		    if (pBuff[5] != 0x16) return false;
            uint8_t sumvalue = CheckSum8(&pBuff[1], 3);
            if (pBuff[4] != sumvalue) return false;
        }
        else
        {
		    if (pBuff[4] != 0x16) return false;
            uint8_t sumvalue = CheckSum8(&pBuff[1], 2);
            if (pBuff[3] != sumvalue) return false;
        }
		
		memcpy(pCtrl, &pBuff[1], 1);
		*pFrameType = eRecvFrameType_Fixed_Length;
		return true;
	}
	
	return false;
}

static int RecvFrame(
	struct config_devices_attr *pDevAttr,
	uint8_t *pBuff,
	int nMaxLen,
	enum eRecvFrameType *pFrameType,
	struct S_CTRL *pCtrl)
{// 接收报文
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	int recvBytes = 0;
	bool bSuccessed = false;//接收成功标志
	while (!bSuccessed)
	{
		if (recvBytes >= nMaxLen)
		{// 接收缓冲区满
			*pFrameType = eRecvFrameType_NULL;
			break;
		}
		
		int tempLen = pContext->pBaseComm->read(pContext->pcom_ctx, pBuff + recvBytes, nMaxLen - recvBytes, pContext->timeout_read);
		if (tempLen <= 0)
		{// 接收数据失败
			*pFrameType = eRecvFrameType_NULL;
			break;
		}
		
		recvBytes += tempLen;
		if (FrameCheckout(pDevAttr, pBuff, recvBytes, pFrameType, pCtrl))
		{// 报文接收成功
			bSuccessed = true;
			break;
		}
	}
	
	return recvBytes;
}

static void Explain_M_SP_NA_1(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 单点信息
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*1 + 2 : byNum*3;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;
	uint8_t byValue = 0;//实时值
	
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 遥信实时值
		byValue = pBuff[offset++];
        if ((byValue & 0x80) != 0)
		{
			if (pDevAttr->debug_enabled) printf("%d:无效, ", wInfoAddr);
			continue;// 无效时跳过
		}

		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, byValue & 0x01);
        SubmitRtValue(pDevAttr, wInfoAddr, byValue & 0x01);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_DP_NA_3(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 双点信息
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*1 + 2 : byNum*3;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;
	uint8_t byValue = 0;//实时值
	
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 遥信实时值
		byValue = pBuff[offset++];
        uint8_t result = byValue & 0x03;
        if ((byValue & 0x80) != 0 || result == 0 || result == 3)
		{
			if (pDevAttr->debug_enabled) printf("%d:无效, result=%d, ", wInfoAddr, result);
			continue;// 无效时跳过
		}

        result = result == 2 ? 1 : 0;// 1确定状态开，2确定状态合
		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, result);
        SubmitRtValue(pDevAttr, wInfoAddr, result);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_ME_NA_9(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 测量值，规一化值
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*3 + 2 : byNum*5;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1)) return;
	
	uint16_t wInfoAddr = 0;//信息体地址
	int nValue = 0;//实时值
	uint8_t byValue = 0;//品质描述
	
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 实时值
		nValue = (short)MAKEWORD(pBuff[offset], pBuff[offset + 1]);
		offset += 2;
		
		// 品质描述
		byValue = pBuff[offset++];
        if ((byValue & 0x80) != 0)
		{
			if (pDevAttr->debug_enabled) printf("%d:无效, ", wInfoAddr);
			continue;// 无效时跳过
		}
		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, nValue);
        SubmitRtValue(pDevAttr, wInfoAddr, nValue);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_ME_NB_11(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 测量值，标度化值
	Explain_M_ME_NA_9(pDevAttr, pBuff, nLen);
}

static void Explain_M_ME_NC_13(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 测量值，短浮点数
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*5 + 2 : byNum*7;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;//信息体地址
	uint8_t byValue = 0;//品质描述
	
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
    float fValue = 0;
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 实时值
        memcpy(&fValue, pBuff + offset, sizeof(fValue));//实时值
		offset += 4;
		
		// 品质描述
		byValue = pBuff[offset++];
        if ((byValue & 0x80) != 0)
		{
			if (pDevAttr->debug_enabled) printf("%d:无效, ", wInfoAddr);
			continue;// 无效时跳过
		}

		if (pDevAttr->debug_enabled) printf("%d:%f, ", wInfoAddr, fValue);
        SubmitRtValue(pDevAttr, wInfoAddr, fValue);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_IT_NA_15(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 累计量
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*5 + 2 : byNum*7;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;//信息体地址
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
    int32_t nValue = 0;
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 实时值
        memcpy(&nValue, pBuff + offset, sizeof(nValue));//实时值
		offset += 4;
		
		// 品质描述
		offset++;
		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, nValue);
        SubmitRtValue(pDevAttr, wInfoAddr, nValue);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_ME_ND_21(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 不带品质描述的归一化值
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*2 + 2 : byNum*4;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;//信息体地址
	int nValue = 0;//实时值
	
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 实时值
		nValue = (short)MAKEWORD(pBuff[offset], pBuff[offset + 1]);
		offset += 2;
		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, nValue);
        SubmitRtValue(pDevAttr, wInfoAddr, nValue);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_M_SP_TB_30(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 带时标CP56Time2a的单点信息
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	
	bool bSQ = (pBuff[1] & 0x80) != 0;
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum == 0)
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 点数为0\n", pContext->phy_id);
		return;
	}
	
	int nDataLen = bSQ ? byNum*8 + 2 : byNum*10;
	if (nDataLen > nLen - 4 - (pDevAttr->addr_size - 1))
	{
		if (pDevAttr->debug_enabled) printf("Rx设备%d: 长度异常，nDataLen=%d, nLen=%d\n", pContext->phy_id, nDataLen, nLen);
		return;
	}
	
	uint16_t wInfoAddr = 0;
	uint8_t byValue = 0;//实时值
	
	int offset = 4 + (pDevAttr->addr_size - 1) + (pDevAttr->cause_size - 1);//第5个字节为信息体地址
	if (pDevAttr->debug_enabled) printf("%d个数据：", byNum);
	for (int i=0; i<byNum; i++)
	{
		if (!bSQ || (bSQ && i==0))
		{// 非顺序记录 或 顺序记录的第一次执行
			wInfoAddr = MAKEWORD(pBuff[offset], pBuff[offset + 1]);
			offset += 2;
		}
		else
		{// 顺序记录，信息体地址递增
			wInfoAddr++;
		}
		
		// 遥信实时值
		byValue = pBuff[offset++];

		// 时标
		offset += 7;
        if ((byValue & 0x80) != 0)
		{
			if (pDevAttr->debug_enabled) printf("%d:无效, ", wInfoAddr);
			continue;// 无效时跳过
		}

		if (pDevAttr->debug_enabled) printf("%d:%d, ", wInfoAddr, byValue & 0x01);
        SubmitRtValue(pDevAttr, wInfoAddr, byValue & 0x01);
	}
	if (pDevAttr->debug_enabled) printf("\n");
}

static void Explain_C_SC_NA_45(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 单点命令
	if (nLen < 7) return;//信息长度错误

	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum != 1) return;

	uint8_t byCot = pBuff[2];//传送原因
	if (byCot != 0x07 && byCot != 0x09) return;//传送原因不正确

	// 信息体地址
	uint16_t wInfoAddr = MAKEWORD(pBuff[4], pBuff[5]);

	// 控制命令码
	uint8_t byDcc = pBuff[6];

	bool bSelect = (byDcc & 0x80) != 0;//遥控选择标志

    struct config_control_attr *pControl = FindControlPt(pDevAttr, wInfoAddr);
	if (pControl == NULL) return;
	if (pControl->double_ctrl) return;//该测点为双点遥控，与返回的类型不符

    bool controlValue = (byDcc & 0x01) != 0;
	if (byCot == 7)
	{// 激活确认
        printf("激活确认成功. bSelect=%d,controlValue=%d\n", bSelect, controlValue);
	}
	else if (byCot == 9)
	{// 停止激活确认
        printf("停止激活确认成功. bSelect=%d,controlValue=%d\n", bSelect, controlValue);
	}
}

static void Explain_C_DC_NA_46(struct config_devices_attr *pDevAttr, uint8_t *pBuff, int nLen)
{// 双点命令
	if (nLen < 7) return;//信息长度错误
	
	uint8_t byNum = pBuff[1] & 0x7f;
	if (byNum != 1) return;
	
	uint8_t byCot = pBuff[2];//传送原因
	if (byCot != 0x07 && byCot != 0x09) return;//传送原因不正确
	
	// 信息体地址
	uint16_t wInfoAddr = MAKEWORD(pBuff[4], pBuff[5]);
	
	// 控制命令码
	uint8_t byDcc = pBuff[6];
	
	bool bSelect = (byDcc & 0x80) != 0;//遥控选择标志
	
    struct config_control_attr *pControl = FindControlPt(pDevAttr, wInfoAddr);
	if (pControl == NULL) return;
	if (!pControl->double_ctrl) return;//该测点为单点遥控，与返回的类型不符
	
    bool controlValue = (byDcc & 0x03) == 0x02;
	if (byCot == 7)
	{// 激活确认
        printf("激活确认成功. bSelect=%d,controlValue=%d\n", bSelect, controlValue);
	}
	else if (byCot == 9)
	{// 停止激活确认
        printf("停止激活确认成功. bSelect=%d,controlValue=%d\n", bSelect, controlValue);
	}
}

static void SubmitRtValue(struct config_devices_attr *pDevAttr, int infAddr, double value)
{
    struct config_context_attr *pContext = &(pDevAttr->context_attr);
    for (int k = 0; k < pDevAttr->node_num; k++)
    {// 根据信息体地址找点位
        struct config_node_attr *pNodeAttr = &(pDevAttr->node_attr[k]);
        if (pNodeAttr->inf_addr != infAddr) continue;
        struct db_spl_remap_info_t *pIndexInfo = pIndexInfo =
            DataMgmtFops.getIndexByPortAndDev(pContext->channel_id, pContext->logic_id);
        if (pIndexInfo == NULL) continue;
		
		double tempValue = value;
		if (pNodeAttr->datattr == 0 || pNodeAttr->datattr == 2)
		{// AI、ACC处理系数a
			tempValue = tempValue * pNodeAttr->kfactor + 0.0005; // +0.0005 四舍五入
		}

		DataMgmtFops.writepoint_in1dev(pIndexInfo, k, &tempValue);
    }
}

static struct config_control_attr *FindControlPt(struct config_devices_attr *pDevAttr, int infAddr)
{// 查找控制点
    for (int k = 0; k < pDevAttr->control_num; k++)
    {// 根据信息体地址找点位
        struct config_control_attr *pControlAttr = &(pDevAttr->control_attr[k]);
        if (pControlAttr->inf_addr == infAddr) return pControlAttr;
    }

    return NULL;
}

static void ProcessCommSucceed(struct config_devices_attr *pDevAttr)
{// 处理通信成功
    struct config_context_attr *pContext = &(pDevAttr->context_attr);
	pContext->failTimes = 0;// 失败次数清零
	pContext->commStatus = eCommStatus_Normal;// 设置为正常状态
	SubmitDevCommStatus(pDevAttr, pContext->commStatus);
}

static void ProcessCommFailed(struct config_devices_attr *pDevAttr)
{// 处理通信失败
    struct config_context_attr *pContext = &(pDevAttr->context_attr);
	if (++pContext->failTimes <= 3) return;// 此处应该开放配置

	pContext->commStatus = eCommStatus_Fault;// 设置为故障状态
	pContext->need_reset = true;// 复位链路标志
	SubmitDevCommStatus(pDevAttr, pContext->commStatus);
}

static void SubmitDevCommStatus(struct config_devices_attr *pDevAttr, enum eCommStatus commStatus)
{
    struct config_context_attr *pContext = &(pDevAttr->context_attr);
	struct db_spl_remap_info_t *pIndexInfo = pIndexInfo =
		DataMgmtFops.getIndexByPortAndDev(pContext->channel_id, pContext->logic_id);
	if (pIndexInfo == NULL) return;

	double tempValue = commStatus == eCommStatus_Normal ? 1 : 0;
	DataMgmtFops.writepoint_in1dev(pIndexInfo, 0, &tempValue);
}
