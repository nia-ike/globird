/*
* Copyright (c) 2013,珠海派诺电子有限公司
* All rights reserved.
*
* 文件名称：
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2020.6.09   V1.00      创建
*/
#include <stdio.h>
#include <string.h>

#include "db_struct.h"

#include "mb_protocol.h"
#include "mb_cfg.h"
#include "mb_com_baseclass.h"
#include "mb_datatype.h"
#include "mb_updatefile.h"

#include "crc16.h"
#include "mb_dlt645_07_com.h"

/*DLT645格式关键字段的长度*/
#define DLT645_DATAID_SIZE			    4
#define DLT645_DATA_FIELD_FACTOR		0x33
#define DLT645_BROADCAST_ADDR_FLAG      0x99
#define DLT645_WILDCARD_FLAG            0xAA
/*数据域的长度，决定数据格式*/
#define DLT645_READDATA_M0              0
#define DLT645_READDATA_M1              1
#define DLT645_READDATA_M6              6

#define DLT645_MAX_R_MAX_LEN            200
#define DLT645_MAX_W_MAX_LEN            50
/* Function codes */
#define DLT645_START_SYMBOL             0x68
#define DLT645_END_SYMBOL               0x16

#define DLT645_OK			0
#define DLT645_ERR			(-1)


/*DLT645 控制码*/
#define DLT645_07_CMD_R_DATA                      0x11
#define DLT645_07_CMD_R_DATA_ACK_FINISHED         0x91
#define DLT645_07_CMD_R_DATA_ACK_UNFINISHED       0xB1
#define DLT645_07_CMD_R_DATA_ACK_ERR              0xD1

#define DLT645_07_CMD_R_COMMADDR                 0x13
#define DLT645_07_CMD_W_DATA                     0x14
#define DLT645_07_CMD_W_COMMADDR                 0x15

/***************************************************************************************************
*对外接口
****************************************************************************************************/
/*DLT645协议异常码*/
#define DLT645_MSG_ERR			(1)
#define DLT645_ID_ERR			(DLT645_MSG_ERR + 1)
#define DLT645_DATA_FIELD_ERR	(DLT645_MSG_ERR + 2)
#define DLT645_FORM_ERR		    (DLT645_MSG_ERR + 3)
#define DLT645_W_PASSWORD_ERR	(DLT645_MSG_ERR + 4)
#define DLT645_EXTERNAL_ERR		(DLT645_MSG_ERR + 5)
#define DLT645_AUTHORITY_ERR	(DLT645_MSG_ERR + 6)
#define DLT645_CMD_DEAL_ERR		(DLT645_MSG_ERR + 7)

#define U32_SIZEOF		            (sizeof(u32))
#define DLT645_U8Buf_swap16(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8))
#define DLT645_U8Buf_swap24(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8) | (U8Buf[2]<<16))
#define DLT645_U8Buf_swap32(U8Buf)	(U8Buf[0] | (U8Buf[1]<<8) |(U8Buf[2]<<16)| (U8Buf[3] << 24))


#define BiglittleSwap16(var)	    (((u16)(var)& 0xff00)>>8) | (((u16)(var) & 0x00ff)<< 8)
#define BiglittleSwap32(var)        (((u32)(var)& 0xff0000)>>24) | (((u32)(var) & 0x00ff0000)>> 8) \
	                              | (((u32)(var) & 0x0000ff00)<< 8) |(((u32)(var) & 0x000000ff)<< 24)


/*控制码的BIT位格式Bit7:传送方向,Bit6应答标记，bit5后续帧标志，bit0~bit4,功能码*/
#define DLT645_C_DIRECTION_BIT			(1<<7)
#define DLT645_C_ACK_BIT				(1<<6)
#define DLT645_C_FOLLOWUP_BIT			(1<<5)

#define DLT645_C_SLAVE_ACK(CTL_CODE)	(CTL_CODE | DLT645_C_DIRECTION_BIT)		
#define DLT645_C_HOST(CTL_CODE)			(CTL_CODE & (~DLT645_C_DIRECTION_BIT))

#define DLT645_C_ACK_OK(CTL_CODE)		(CTL_CODE & (~DLT645_C_ACK_BIT))		
#define DLT645_C_ACK_ERR(CTL_CODE)		(CTL_CODE | DLT645_C_ACK_BIT)

#define DLT645_C_Y_FOLLOWUP(CTL_CODE)	(CTL_CODE | DLT645_C_FOLLOWUP_BIT)		
#define DLT645_C_N_FOLLOWUP(CTL_CODE)	(CTL_CODE & (~DLT645_C_FOLLOWUP_BIT))


/**********************************************************************************
*以下结构请参见通信协议DLT645-2007
*********************************************************************************/
#pragma pack(push,1)

typedef struct  tagDLT645_2007_FrameHead
{
	uint8_t StartSymbol;						/*帧起始符*/
	uint8_t CommAddr[DLT645_COMM_ADDR_SIZE];	/*地址域*/
	uint8_t SecSymbol;							/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
}DLT645_FRAME_HEAD;

typedef struct  tagDLT645_2007_FrameEnd
{
	uint8_t Cs;					/*校验码*/
	uint8_t EndSymbol;			/*结束符*/
}DLT645_FRAME_END;

#pragma pack(pop)

typedef struct  tagDlt645_2007_Packet
{
	DLT645_FRAME_HEAD   *pFrameHead;
	uint8_t         	*pFrameData;	/*数据域*/ 
	DLT645_FRAME_END  	*pFrameEnd;		
}DLT645_PACKET;

#define DLT645_HEAD_SIZE				(sizeof(DLT645_FRAME_HEAD))		
#define DLT645_CRC_SIZE                 (sizeof(uint8_t))        
#define DLT645_ENDFLAG_SIZE             (sizeof(uint8_t))

#define DLT645_PACK_SIZE(DataLen)		((DataLen) + DLT645_HEAD_SIZE + DLT645_CRC_SIZE + DLT645_ENDFLAG_SIZE)	
#define DLT645_CS_COUNT_SIZE(DataLen)   ((DataLen) + DLT645_HEAD_SIZE )	

/*************读操作命令的数据段的格式如下***************************************/
typedef struct tagDlt645_DataField_R_M0/*数据长度为04H+0*/
{
	uint8_t DataId[DLT645_DATAID_SIZE];
}DLT645_Read_Data;

typedef struct tagDlt645_DataField_err
{
	uint8_t ERR;
}DLT645_ACK_ERR;




ST_USER_CFG_INTF gstIntfCfg;
#define DLT645_OPERATE_R_COMMADDR			0xFFFF0E09	/*读通信地址操作码*/


/*************************************************
    函数名称:       Dlt645_user_register_intf
    功能描述:       用户配置表接口
    子 函 数:       
    调用限制:       无
    输入参数:      pUserTable :上层配置的变量表
    输出参数:       usSize，记录条数
    返 回 值:       
    说    明:       
*************************************************/
int32_t Dlt645_user_register_intf(ST_USER_CFG_INTF *pUserInfo)
{

	if(NULL == pUserInfo)
	{
		return DLT645_PARAM_ERR;
	}
	memcpy((uint8_t*)&gstIntfCfg,(uint8_t*)pUserInfo,sizeof(ST_USER_CFG_INTF));

	return DLT645_OK;
}

/************************************************************************
函数功能：Dlt645_crc   DLT645计算CRC
输入参数：pchData  计算的字符串
          dwLength 长度
输出参数：
返回值  ：CRC的值
说明    ：
************************************************************************/
uint8_t Dlt645_crc(uint8_t *pchData,uint8_t dwLength)
{
    uint8_t wCrc = 0;
    uint8_t i;
	
    for( i = 0 ; i < dwLength ; i++ )
    {
        wCrc += *(pchData + i);
    }
    return wCrc;
}
/************************************************************************
    函数名称:       Dlt645_leave_factor_deal
    功能描述:       接收方按字节进行减33H处理
    输入参数:      
    输出参数:       pDataField，返回报文的指针
    返 回 值:       
    说    明:       读数据
************************************************************************/
void Dlt645_leave_factor_deal(uint8_t *pData,uint8_t DataLen)
{
    uint16_t i = 0;
    for(i = 0;i < DataLen; i++ )
    {
        pData[i] -= DLT645_DATA_FIELD_FACTOR;
    }
}
/************************************************************************
    函数名称:       Dlt645_add_factor_deal
    功能描述:       发送方按字节进行加33H处理
    输入参数:       pBackPart，后半部分数据处理（除了头部ST_DLT645_2007_HEAD）
                   DataLen，后半部分的数据长度；
    输出参数:       pDataField，返回报文的指针
    返 回 值:       
    说    明:       读数据
************************************************************************/
void Dlt645_add_factor_deal(uint8_t *pData,uint8_t DataLen)
{
    uint16_t i = 0;
    for(i = 0;i < DataLen; i++ )
    {
        pData[i] += DLT645_DATA_FIELD_FACTOR;
    }
}

/************************************************************************
函数功能：Dlt645_check_legality_packet   检测数据包的合法性
输入参数：pPacket  接收到的数据包；
          Plen 长度
输出参数：
返回值  :返回ACK包的长度；为0表示异常或不处理
说明    ：
************************************************************************/
int32_t Dlt645_check_legality_packet(const DLT645_PACKET *pstRecMsg,uint16_t Plen)
{
	uint8_t ucCs = 0;
    uint16_t usCsOff = 0;

	if(NULL == pstRecMsg)
	{
		return DLT645_ERR;
	}

	if((DLT645_START_SYMBOL != pstRecMsg->pFrameHead->StartSymbol)
		||(DLT645_START_SYMBOL !=  pstRecMsg->pFrameHead->SecSymbol))
	{
		return DLT645_ERR;
	}

	usCsOff = DLT645_CS_COUNT_SIZE(pstRecMsg->pFrameHead->DataLen);

	ucCs = Dlt645_crc((uint8_t*)pstRecMsg->pFrameHead,usCsOff);

	if((ucCs != pstRecMsg->pFrameEnd->Cs)
		||(DLT645_END_SYMBOL != pstRecMsg->pFrameEnd->EndSymbol))
	{
		return DLT645_ERR;
	}
	
	return DLT645_OK;
}
/************************************************************************
函数功能：Dlt645_check_ID   检测接收到的地址为合法
输入参数：pAddr  地址数据——6个字节
          Plen 长度
输出参数：
返回值  ：CRC的值
说明    ：
************************************************************************/
int32_t Dlt645_check_ID(const uint8_t *pAddr)
{
	uint8_t i = 0;
	
	if(NULL  == pAddr)
	{
		return DLT645_PARAM_ERR;
	}

	for(i = 0; i< DLT645_COMM_ADDR_SIZE; i++)
	{
		if(DLT645_BROADCAST_ADDR_FLAG != pAddr[i])
		{
			break;
		}
	}
	if(DLT645_COMM_ADDR_SIZE == i)
	{/*广播地址*/
		return DLT645_OK;
	}

	for(i = 0; i< DLT645_COMM_ADDR_SIZE; i++)
	{
		if((DLT645_WILDCARD_FLAG  ==  pAddr[i])
            || (DLT645_WILDCARD_FLAG  ==  gstIntfCfg.CommAddr[i]))
        {
            continue;
        }
        if(gstIntfCfg.CommAddr[i] != pAddr[i])
		{
			break;
		}
	}

	if(DLT645_COMM_ADDR_SIZE != i)
	{
		return DLT645_ERR;
	}

	return DLT645_OK;

}

/*************************************************
    函数名称:       Dlt645_ack_err_msg
    功能描述:       异常返回报文,生成
    子 函 数:       
    调用限制:       无
    输入参数:       trans，返回报文的指针
                    err_code，异常码
    输出参数:       trans，返回报文的指针
    返 回 值:       返回报文的长度
    说    明:       无
*************************************************/
uint16_t Dlt645_ack_err_msg(DLT645_PACKET *pAckMsg,uint8_t Err)
{
	DLT645_ACK_ERR *pstAckErr =(DLT645_ACK_ERR*) pAckMsg->pFrameData;
	pAckMsg->pFrameEnd =(DLT645_FRAME_END *)((uint8_t*) pAckMsg->pFrameData + sizeof(DLT645_ACK_ERR));

	pAckMsg->pFrameHead->CtlCode = DLT645_C_SLAVE_ACK(pAckMsg->pFrameHead->CtlCode);
	pAckMsg->pFrameHead->CtlCode = DLT645_C_ACK_ERR(pAckMsg->pFrameHead->CtlCode);
	pAckMsg->pFrameHead->CtlCode = DLT645_C_N_FOLLOWUP(pAckMsg->pFrameHead->CtlCode);
	pAckMsg->pFrameHead->DataLen = sizeof(uint8_t);

	pstAckErr->ERR = Err + DLT645_DATA_FIELD_FACTOR;

	pAckMsg->pFrameEnd->Cs =Dlt645_crc((uint8_t*)pAckMsg->pFrameHead,DLT645_CS_COUNT_SIZE(pAckMsg->pFrameHead->DataLen));
	pAckMsg->pFrameEnd->EndSymbol = DLT645_END_SYMBOL;
	return DLT645_PACK_SIZE(pAckMsg->pFrameHead->DataLen);
}
/*************************************************
    函数名称:       Dlt645_ack_ok_msg
    功能描述:       返回OK报文
    子 函 数:       
    调用限制:       无
    输入参数:       trans，返回报文的指针
                    err_code，异常码
    输出参数:       trans，返回报文的指针
    返 回 值:       返回报文的长度
    说    明:       无
*************************************************/
uint16_t Dlt645_ack_ok_msg(DLT645_PACKET *pAckMsg)
{
	pAckMsg->pFrameHead->DataLen = 0;
	pAckMsg->pFrameEnd->Cs =Dlt645_crc((uint8_t*)pAckMsg->pFrameHead,DLT645_CS_COUNT_SIZE(pAckMsg->pFrameHead->DataLen));
	pAckMsg->pFrameEnd->EndSymbol = DLT645_END_SYMBOL;
	return DLT645_PACK_SIZE(pAckMsg->pFrameHead->DataLen);
}
/************************************************************************
    函数名称:       Dlt645_M0_read_deal
    功能描述:       读数据功能码处理
    输入参数:       pAckMsg，帧格式
                    pstM0，数据标识码；
    输出参数:       pAckMsg，帧格式
    返 回 值:       返回报文的长度
    说    明:       读数据
************************************************************************/
uint16_t Dlt645_M0_read_deal(DLT645_Read_Data *pstM0,DLT645_PACKET *pAckMsg)
{
	uint16_t	usLen = 0;
	uint32_t OperateCode  = 0;
	uint8_t *pOutData = NULL;
	
	if((NULL == pstM0)
		||(NULL == pAckMsg))
	{
		return 0;
    }
		
    memcpy((uint8_t *)pAckMsg->pFrameData,(uint8_t *)&pstM0->DataId[0],DLT645_DATAID_SIZE);/*载入DATAID*/
    
    pOutData = pAckMsg->pFrameData + DLT645_DATAID_SIZE;
	usLen = DLT645_MAX_R_MAX_LEN - DLT645_PACK_SIZE(DLT645_DATAID_SIZE);
	
	OperateCode = DLT645_U8Buf_swap32(pstM0->DataId);
	usLen = gstIntfCfg.pR_Fun(OperateCode,NULL,DLT645_READDATA_M0,pOutData,usLen);

	/*计算CRC及刷新响应帧数据*/
	pAckMsg->pFrameHead->DataLen = DLT645_DATAID_SIZE + usLen;
	if(0 == usLen)
	{
		pAckMsg->pFrameHead->CtlCode = DLT645_07_CMD_R_DATA_ACK_ERR;
		pAckMsg->pFrameHead->DataLen = 0x01;
		pAckMsg->pFrameData[0] = DLT645_ID_ERR; 
//        return 0;
	}

	Dlt645_add_factor_deal(pAckMsg->pFrameData,pAckMsg->pFrameHead->DataLen);

	usLen = DLT645_CS_COUNT_SIZE(pAckMsg->pFrameHead->DataLen);
	pAckMsg->pFrameEnd = (DLT645_FRAME_END *)((uint8_t *)pAckMsg->pFrameData + pAckMsg->pFrameHead->DataLen);
	pAckMsg->pFrameEnd->Cs = Dlt645_crc((uint8_t *)pAckMsg->pFrameHead,usLen);
	pAckMsg->pFrameEnd->EndSymbol = DLT645_END_SYMBOL;

	return DLT645_PACK_SIZE(pAckMsg->pFrameHead->DataLen);
}


/************************************************************************
    函数名称:       Dlt645_read_deal
    功能描述:       读数据功能码处理
    输入参数:        pData，数据标识
					pAckMsg，请求帧
                    DataLen，数据域长度；
    输出参数:        pAckMsg，请求帧的指针
    返 回 值:       返回报文的长度
    说    明:       读数据

    .  DLT645包帧标准格式
    ·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符
    ·　DLT645读数据的包格式如下：
    ·　功能：请求读电能表数据
    ·　控制码：C=11H
    ·　数据域长度：L=04H+m（数据长度）
    ·　（m=0）： 数据域：DIODI1DI2DI3（数据标识）          
    ·　（m=1）： 数据域：DIODI1DI2DI3（数据标识）+ N（负荷记录块数）
    ·　（m=6）： 数据域：DIODI1DI2DI3（数据标识） + N（负荷记录块数）+分|时|日|月|年
************************************************************************/
uint16_t Dlt645_read_deal(uint8_t *pData,uint8_t DataLen, DLT645_PACKET *pAckMsg)
{
	uint16_t usLen = 0; 
	if((NULL == pData)
		||(DLT645_DATAID_SIZE > DataLen))
	{/*内部异常:不响应*/
		return 0;
	}

	/*数据域的长度决定数据格式*/
	if(DLT645_READDATA_M0 == (DataLen - DLT645_DATAID_SIZE))
	{
		usLen = Dlt645_M0_read_deal((DLT645_Read_Data *)pData,pAckMsg);
	}
	else
	{
		return Dlt645_ack_err_msg(pAckMsg,DLT645_DATA_FIELD_ERR);
	}
	return usLen;
}
/*************************************************
    函数名称:       Dlt645_read_ComAddr_deal
    功能描述:       读通信地址处理
    子 函 数:       
    调用限制:       无
    输入参数:     pAckMsg响应包 
    输出参数:       
    返 回 值:       返回报文的长度
    说    明:       读数据
*************************************************/
uint16_t Dlt645_read_ComAddr_deal(DLT645_PACKET *pAckMsg)
{
	uint16_t usLen = 0; 

	if(NULL == pAckMsg)
	{/*内部异常:不响应*/
		return 0;
	}
   
	usLen = gstIntfCfg.pR_Fun(DLT645_OPERATE_R_COMMADDR,NULL,0,(void *)pAckMsg->pFrameData,DLT645_COMM_ADDR_SIZE);
	if(DLT645_COMM_ADDR_SIZE != usLen)
	{/*读取失败时,取缓存的*/
        memcpy((uint8_t*)pAckMsg->pFrameHead->CommAddr,gstIntfCfg.CommAddr,DLT645_COMM_ADDR_SIZE);
		memcpy((uint8_t*)pAckMsg->pFrameData,gstIntfCfg.CommAddr,DLT645_COMM_ADDR_SIZE);	
	}
	else
	{
		memcpy((uint8_t*)pAckMsg->pFrameHead->CommAddr,pAckMsg->pFrameData,DLT645_COMM_ADDR_SIZE);
		memcpy((uint8_t*)gstIntfCfg.CommAddr,pAckMsg->pFrameData,DLT645_COMM_ADDR_SIZE);		
	}
		
    Dlt645_add_factor_deal(pAckMsg->pFrameData,DLT645_COMM_ADDR_SIZE);
		/*计算CRC及刷新响应帧数据*/
	pAckMsg->pFrameHead->DataLen = DLT645_COMM_ADDR_SIZE;
	
    usLen = DLT645_CS_COUNT_SIZE(pAckMsg->pFrameHead->DataLen);
	pAckMsg->pFrameEnd = (DLT645_FRAME_END *)((uint8_t *)pAckMsg->pFrameData + pAckMsg->pFrameHead->DataLen);
	pAckMsg->pFrameEnd->Cs = Dlt645_crc((uint8_t *)pAckMsg->pFrameHead,usLen);
	pAckMsg->pFrameEnd->EndSymbol = DLT645_END_SYMBOL;
	return DLT645_PACK_SIZE(pAckMsg->pFrameHead->DataLen);
}

/************************************************************************
函数功能：Dlt645_analyze_packet_deal   分解数据处理
输入参数：pPacket  接收到的数据包；
          Plen 长度
输出参数：
返回值  :返回ACK包的长度；为0表示异常或不处理
说明    ：
************************************************************************/
uint16_t Dlt645_analyze_packet_deal(uint8_t *pRevBuf,uint16_t Revlen,uint8_t *pAckPack,uint16_t Buflen)
{
	DLT645_PACKET stRecMsg ={0};
	DLT645_PACKET stAckMsg = {0};
	uint16_t usCsOff = 0;
	uint16_t AckPackLen = 0;
	int32_t Ret = 0;
	uint16_t i = 0;
    uint16_t RecLen = 0;
	uint16_t j=0;
//	printf("%s %d\r\n", __FILE__, __LINE__);
    
	if((NULL == pRevBuf)
		||(NULL == pAckPack)
		||(Revlen < DLT645_PACK_SIZE(0)))
	{/*备注：pPostAction可为空*/
		return 0;
	}

    /*跳过前导字符0xFE*/
    for(i = 0;i < Revlen; i++)
    {
        if(0xfe != pRevBuf[i])
             break;
    }

    if(i >= Revlen)
    {
        /*非法包直接不响应*/
		return 0; 
    }
//	printf("%s %d\r\n", __FILE__, __LINE__);
    
    RecLen = Revlen - i;
     
	stRecMsg.pFrameHead = (DLT645_FRAME_HEAD *)(&pRevBuf[i]);
	stRecMsg.pFrameData = (uint8_t *)((uint8_t *)stRecMsg.pFrameHead + DLT645_HEAD_SIZE);

	usCsOff = DLT645_CS_COUNT_SIZE(stRecMsg.pFrameHead->DataLen);
	stRecMsg.pFrameEnd= (DLT645_FRAME_END *)((uint8_t *)stRecMsg.pFrameHead + usCsOff);

	Ret = Dlt645_check_legality_packet(&stRecMsg,RecLen);
//	printf("Dlt645_check_legality_packet->Ret:%d \n",Ret);
	if(DLT645_OK != Ret)
	{/*非法包直接不响应*/
		return 0;
	}
//	printf("%s %d\r\n", __FILE__, __LINE__);
	
	stAckMsg.pFrameHead = (DLT645_FRAME_HEAD *)pAckPack;
	stAckMsg.pFrameData = (uint8_t*) (pAckPack + DLT645_HEAD_SIZE);
	stAckMsg.pFrameEnd =NULL;
	memcpy((uint8_t*)stAckMsg.pFrameHead ,(uint8_t*)stRecMsg.pFrameHead,DLT645_HEAD_SIZE);/*填充ACK包的公共信息*/
 
    /*刷新响应包的头信息默认配置*/
	stAckMsg.pFrameHead->CtlCode = DLT645_C_SLAVE_ACK(stAckMsg.pFrameHead->CtlCode);
	stAckMsg.pFrameHead->CtlCode = DLT645_C_ACK_OK(stAckMsg.pFrameHead->CtlCode);
	stAckMsg.pFrameHead->CtlCode = DLT645_C_N_FOLLOWUP(stAckMsg.pFrameHead->CtlCode);
	stAckMsg.pFrameHead->DataLen = Buflen - DLT645_PACK_SIZE(0);/*最大响应数据*/

    if((DLT645_07_CMD_R_COMMADDR != stRecMsg.pFrameHead->CtlCode)
        &&(DLT645_07_CMD_W_COMMADDR != stRecMsg.pFrameHead->CtlCode))
    {
        Ret = Dlt645_check_ID(stRecMsg.pFrameHead->CommAddr);
        if(DLT645_OK != Ret)
        {
            return 0;
            //return Dlt645_ack_err_msg(&stAckMsg,DLT645_ID_ERR);
        }
    }

	/*统一处理数据域的-0x33*/
	Dlt645_leave_factor_deal(stRecMsg.pFrameData,stRecMsg.pFrameHead->DataLen);

	/*根据控制码区分处理*/
	switch(stRecMsg.pFrameHead->CtlCode)
	{
		case DLT645_07_CMD_R_DATA:
			{
				AckPackLen = Dlt645_read_deal(stRecMsg.pFrameData,stRecMsg.pFrameHead->DataLen,&stAckMsg);
			}
			break;
		case DLT645_07_CMD_R_COMMADDR:
            {
                AckPackLen = Dlt645_read_ComAddr_deal(&stAckMsg);
            }
            break;
		default:
			break;
	}
	return AckPackLen;
}



