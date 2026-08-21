#include "iec101-send.h"
#include "iec101-data.h"
#include "iec101-utils.h"

#define _DEBUG      DEBUG_OFF

void SendFrameFix(struct config_devices_attr *pDevAttr, uint8_t fun)
{// 发送固定长度帧
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	struct M_CTRL txctrl;
	memset(&txctrl, 0, 1);
	txctrl.FUN = fun;
	txctrl.PRM = 1;
	txctrl.FCB = pContext->fcb_flag;
	txctrl.FCV = 1;
	pContext->fcb_flag = !pContext->fcb_flag;

	SendFrameCtrl(pDevAttr, &txctrl);
}

void SendFrameCtrl(struct config_devices_attr *pDevAttr, struct M_CTRL *pCtrl)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	uint8_t Sendbuff[6] = { 0 };
    int offset = 0;
	Sendbuff[offset++] = 0x10;
	memcpy(&Sendbuff[offset++], pCtrl, 1);
	Sendbuff[offset++] = pContext->phy_id;
    if (pDevAttr->addr_size == 2) Sendbuff[offset++] = 0;
	Sendbuff[offset++] = CheckSum8(&Sendbuff[1], pDevAttr->addr_size + 1); 
	Sendbuff[offset++] = 0x16;
	
    printf("xxxxxxxxxxxxxxxxx\n");
	usleep(100E3);
	pContext->pBaseComm->write(pContext->pcom_ctx, Sendbuff, offset);
	usleep(100E3);
}

void SendFrame(struct config_devices_attr *pDevAttr, uint8_t fun, uint8_t *pASDU, uint8_t len)
{
	struct config_context_attr *pContext = &(pDevAttr->context_attr);

	struct M_CTRL txctrl;
	memset(&txctrl, 0, 1);
	txctrl.FUN = fun;
	txctrl.PRM = 1;
	txctrl.FCB = pContext->fcb_flag;
	txctrl.FCV = 1;
	pContext->fcb_flag = !pContext->fcb_flag;

	uint8_t Sendbuff[len + 9];
    int offset = 0;
	Sendbuff[offset++] = 0x68;
	Sendbuff[offset++] = len + 2 + (pDevAttr->addr_size - 1);   //长度
	Sendbuff[offset++] = len + 2 + (pDevAttr->addr_size - 1);   //长度
	Sendbuff[offset++] = 0x68;
	memcpy(&Sendbuff[offset++], &txctrl, 1);
	Sendbuff[offset++] = pContext->phy_id;
    if (pDevAttr->addr_size == 2) Sendbuff[offset++] = 0;
	
	memcpy(&Sendbuff[offset], pASDU, len); 
    offset += len;
	Sendbuff[offset++] = CheckSum8(&Sendbuff[4], len + pDevAttr->addr_size + 1);
	Sendbuff[offset++] = 0x16;
	usleep(100E3);
	pContext->pBaseComm->write(pContext->pcom_ctx, Sendbuff, offset);
	usleep(100E3);
}
