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
#include "iec101-cfg.h"
#include "iec101-process.h"

#include "pkt.h"
#include "iec101-send.h"
#include "iec101-data.h"
#include "iec101-recv.h"
#include "iec101-utils.h"

void AdjustTime(struct config_devices_attr *pDevAttr)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	uint8_t BuffAsdu[14]; 
    int offset = 0;
	BuffAsdu[offset++] = 0x67;  //ASDU103
	BuffAsdu[offset++] = 0x01;  //可变结构限定词
	BuffAsdu[offset++] = 0x06;  //传送原因
    if (pDevAttr->cause_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = pContext->phy_id; //公共地址
    if (pDevAttr->addr_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = 0x00;
	BuffAsdu[offset++] = 0x00;  //信息地址
	
    time_t tempTmNow = time(NULL);
    struct tm *tmNow = localtime(&tempTmNow);
	BuffAsdu[offset++] = LOBYTE((uint16_t)(tmNow->tm_sec * 1000));
	BuffAsdu[offset++] = HIBYTE((uint16_t)(tmNow->tm_sec * 1000));
	BuffAsdu[offset++] = (uint8_t)tmNow->tm_min;;
	BuffAsdu[offset++] = (uint8_t)tmNow->tm_hour;
	BuffAsdu[offset++] = (uint8_t)tmNow->tm_mday;
	BuffAsdu[offset++] = (uint8_t)tmNow->tm_mon;
	BuffAsdu[offset++] = (uint8_t)(tmNow->tm_year % 100);
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 发送校时\n", pContext->phy_id);
	SendFrame(pDevAttr, 0x03, BuffAsdu, offset);

	usleep(500E3);
	
	struct S_CTRL ctrl;
    for (int i = 0; i < 20; i++)
    {// 最多20次查询一级数据，直到查询完毕
        if (!RecvAndExplainFrame(pDevAttr, &ctrl)) break;
        if (ctrl.ACD == 0) break;
		if (pDevAttr->debug_enabled) printf("Tx设备%d: 召唤一级数据:i=%d\n", pContext->phy_id, i);
        SendFrameFix(pDevAttr, 0x0a);// 召唤一级数据
    }
}

void Query_AllStart(struct config_devices_attr *pDevAttr)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

    int offset = 0;
	uint8_t BuffAsdu[10];	//组织ASDU结构
	BuffAsdu[offset++] = C_IC_NA_1; //ASDU100总召唤
	BuffAsdu[offset++] = 0x01;	//可变结构限定词
	BuffAsdu[offset++] = 0x06;	//传送原因：激活
    if (pDevAttr->cause_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = pContext->phy_id;  //公共地址
    if (pDevAttr->addr_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = 0x00;
	BuffAsdu[offset++] = 0x00;
	BuffAsdu[offset++] = 0x14;
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 发送总召\n", pContext->phy_id);
	SendFrame(pDevAttr, 0x03, BuffAsdu, offset);

	struct S_CTRL ctrl;
    for (int i = 0; i < 20; i++)
    {// 最多20次查询一级数据，直到查询完毕
        if (!RecvAndExplainFrame(pDevAttr, &ctrl)) break;
        if (ctrl.ACD == 0) break;
		if (pDevAttr->debug_enabled) printf("Tx设备%d: 召唤一级数据:i=%d\n", pContext->phy_id, i);
        SendFrameFix(pDevAttr, 0x0a);// 召唤一级数据
    }
}

void Query_AllAccStart(struct config_devices_attr *pDevAttr)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

    int offset = 0;
	uint8_t BuffAsdu[8];	//组织ASDU结构
	BuffAsdu[offset++] = C_CI_NA_1; //101-电能脉冲召唤命令
	BuffAsdu[offset++] = 0x01;	//可变结构限定词
	BuffAsdu[offset++] = 0x06;	//传送原因：激活
    if (pDevAttr->cause_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = pContext->phy_id;  //公共地址
    if (pDevAttr->addr_size == 2) BuffAsdu[offset++] = 0;
	BuffAsdu[offset++] = 0x00;
	BuffAsdu[offset++] = 0x00;
	BuffAsdu[offset++] = 0x14;
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 发送电能脉冲总召\n", pContext->phy_id);
	SendFrame(pDevAttr, 0x03, BuffAsdu, offset);

	struct S_CTRL ctrl;
    for (int i = 0; i < 20; i++)
    {// 最多20次查询一级数据，直到查询完毕
        if (!RecvAndExplainFrame(pDevAttr, &ctrl)) break;
        if (ctrl.ACD == 0) break;
		if (pDevAttr->debug_enabled) printf("Tx设备%d: 召唤一级数据:i=%d\n", pContext->phy_id, i);
        SendFrameFix(pDevAttr, 0x0a);// 召唤一级数据
    }
}

// 复位链路
bool ResetLinkState(struct config_devices_attr *pDevAttr)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

    struct M_CTRL m_txctrl;
	memset(&m_txctrl, 0, 1);
	struct S_CTRL ctrl;
	// 复位链路状态
	m_txctrl.FUN = 9;
	m_txctrl.PRM = 1;
	m_txctrl.FCB = 0;
	m_txctrl.FCV = 0;
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 发送复位链路\n", pContext->phy_id);
	SendFrameCtrl(pDevAttr, &m_txctrl);// 复位链路状态
	if (!RecvAndExplainFrame(pDevAttr, &ctrl)) return false;
	
	// 复位远方链路状态
	m_txctrl.FUN = 0;
	m_txctrl.PRM = 1;
	m_txctrl.FCB = 0;
	m_txctrl.FCV = 0;
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 复位远方链路状态\n", pContext->phy_id);
	SendFrameCtrl(pDevAttr, &m_txctrl);// 复位远方链路状态
	if (!RecvAndExplainFrame(pDevAttr, &ctrl)) return false;

	return true;
}

void ProcessRtData(struct config_devices_attr *pDevAttr)
{// 处理实时数据
	struct config_context_attr *pContext = &(pDevAttr->context_attr);
	struct S_CTRL ctrl;
	if (pDevAttr->debug_enabled) printf("Tx设备%d: 召唤二级数据\n", pContext->phy_id);
	SendFrameFix(pDevAttr, 0x0b);// 召唤二级数据
    for (int i = 0; i < 20; i++)
    {// 最多20次
        if (!RecvAndExplainFrame(pDevAttr, &ctrl)) break;
        if (ctrl.ACD == 0) break;
		if (pDevAttr->debug_enabled) printf("Tx设备%d: 召唤一级数据:i=%d\n", pContext->phy_id, i);
        SendFrameFix(pDevAttr, 0x0a);// 召唤一级数据
    }
}

void ProcessCmd(struct config_devices_attr *pDevAttr)
{// 处理命令
	// if (!m_pDevStruct->m_devObj->YkEnable) return;//当前设备不允许遥控操作
	// RTCmdStruct cmd;
	// while (this->PopNewCmd(cmd))
	// {// 存在新的命令
	// 	if (cmd.m_CmdType != eCmdType_YkSelect &&
	// 		cmd.m_CmdType != eCmdType_YkExecute &&
	// 		cmd.m_CmdType != eCmdType_YkCancel) continue;

	// 	CTblYk *tbl = m_pDevStruct->GetYkByID(cmd.m_PtId);
	// 	if (tbl == NULL) continue;

	// 	byte byDcc = 0x04;//控制命令码
	// 	byte byCot = 0x06;//传送原因
	// 	MyString strTip = "";
	// 	if (cmd.m_CmdType == eCmdType_YkSelect)
	// 	{// 遥控选择
	// 		byDcc |= 0x80;
	// 		strTip = "遥控选择命令";
	// 	}
	// 	else if (cmd.m_CmdType == eCmdType_YkExecute)
	// 	{
	// 		strTip = "遥控执行命令";
	// 	}
	// 	else if (cmd.m_CmdType == eCmdType_YkCancel)
	// 	{
	// 		byCot = 0x08;
	// 		strTip = "遥控撤销命令";
	// 	}

	// 	if (cmd.m_bValue)
	// 	{// 合闸
	// 		byDcc |= (tbl->IsDoubleCtrl ? 0x02 : 0x01);
	// 	}
	// 	else
	// 	{// 分闸
	// 		byDcc |= (tbl->IsDoubleCtrl ? 0x01 : 0x00);
	// 	}

	// 	byte BuffAsdu[7];		//组织ASDU结构
	// 	BuffAsdu[0] = (byte)(tbl->IsDoubleCtrl ? 46 : 45);//类型标识
	// 	BuffAsdu[1] = 0x01;		//可变结构限定词
	// 	BuffAsdu[2] = byCot;	//传送原因
	// 	BuffAsdu[3] = m_pDevStruct->m_devObj->DevAddr;	//公共地址
	// 	BuffAsdu[4] = LOBYTE(tbl->IOAddr);
	// 	BuffAsdu[5] = HIBYTE(tbl->IOAddr);//遥控点号 INF
	// 	BuffAsdu[6] = byDcc;	//断路器控制命令DCC
	// 	SendFrame(0x03, BuffAsdu, 7, strTip);
	// 	msleep(300);

	// 	S_CTRL ctrl;
	// 	while (RecvAndExplainFrame(ctrl) && (ctrl.ACD != 0))
	// 	{// 召唤一级数据
	// 		if (TopVal::Instance()->m_bExit) break;
	// 		SendFrame(0x0a);
	// 	}
	// }
}
