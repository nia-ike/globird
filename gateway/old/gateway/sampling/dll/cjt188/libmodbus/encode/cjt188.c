
#include "cjt188.h"
#include "crc16.h"
#include <math.h>


#define _DEBUG      DEBUG_OFF

#define CJT188_PACK_SIZE(DataLen)		((DataLen) + CJT188_COMMONFORM_SIZE + CJT188_CRC_SIZE + CJT188_ENDFLAG_SIZE)	
#define CJT188_CS_COUNT_SIZE(DataLen)   ((DataLen) + CJT188_COMMONFORM_SIZE )	

#define MB_ALIGN_PT_8BIT_READ(p)        (*((uint8_t *)(p))) 
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)
#define MB_ALIGN_PT_32BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff; (*((uint8_t*)p+2)) = v & 0xff;  (*((uint8_t*)p+3)) = v & 0xff;}while(0)


/************************************************************************
函数功能：      SetAddr()
功能描述:       地址域
输入参数:        
输出参数:       
返 回 值:      
说    明:       
************************************************************************/
int32_t SetAddr(uint8_t *pAddr, uint64_t wAddr)
{
    uint8_t i;
    uint64_t temp;
    for(i=0; i<CJT188_COMM_ADDR_SIZE; i++)
    {
        temp = wAddr%100;
        pAddr[i] = ((temp/10)<<4) + ((temp%10) & 0x0F);
        wAddr /= 100;
    }   
    return 0;
}

/************************************************************************
函数功能：      SetDataId()
功能描述:       数据标识
输入参数:       
输出参数:       
返 回 值:       
说    明:       
************************************************************************/
int32_t SetDataId(uint8_t *pDataId, uint32_t wDataId)
{
    uint8_t i;

    for( i = 0 ; i < CJT188_DATAID_SIZE ; i++ )
    {        
        pDataId[i] = (uint8_t)((wDataId & (0xFF << (8*i))) >> (8*i));
    }
    return 0;
}

/************************************************************************
函数功能：   CalcSum()
功能描述:       和校验码
输入参数:        pBuf,校验的数据
                wSize,校验的数据的个数
输出参数:       
返 回 值:       返回和校验码
说    明:       

************************************************************************/
int32_t AddExData(uint8_t *pBuff, uint8_t *pExContext, uint8_t wExContextLen)
{
    if( wExContextLen > 200 )
        return -1;
    LPCJT188ReadREG01H  pDLT645ReadData;
    pDLT645ReadData = (LPCJT188ReadREG01H )(pBuff) ;
    if( wExContextLen > 200 )
        return -1;
    memcpy((uint8_t *)(&pDLT645ReadData->DataId[0] + sizeof(&pDLT645ReadData->DataId)), pExContext, (wExContextLen));
    pDLT645ReadData->DataLen = sizeof(pDLT645ReadData->DataId) + wExContextLen;
    return pDLT645ReadData->DataLen;
    return 0;
}


/************************************************************************
函数功能：      CalcSum()
功能描述:       和校验码
输入参数:        pBuf,校验的数据
                wSize,校验的数据的个数
输出参数:       
返 回 值:       返回和校验码
说    明:       
************************************************************************/
/*和校验码*/
uint8_t CalcSum(uint8_t *pBuf, uint16_t wSize)
{
    uint16_t i = 0;
    uint8_t sum = 0;
    for(i = 0; i<wSize;i++)
    {
        sum += pBuf[i];
    }
    return sum;
}

/************************************************************************
函数功能：   Packet01H()
功能描述:       读数据功能码处理
输入参数:        pData，数据标识
                pAckMsg，请求帧
                DataLen，数据域长度；
输出参数:        pAckMsg，请求帧的指针
返 回 值:       返回报文的长度
说    明:       读数据

.  CJT188包帧标准格式
·　帧格式：| 帧起始符 | 仪表类型 | 地址域 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符 |
·　CJT188读数据的包格式如下：
·　功能：请求读计量数据
·　控制码：C=01H
·　数据域长度：L=03H（数据长度）

    68 10 03 00 01 20 00 00 00 01 03 90 1F 01 50 16
    68 -- 帧起始符；
    10 -- 表计类型代码；
    03 00 01 20 00 00 00 -- 表计地址；
    01 -- 控制码；
    03 -- 数据域长度； 
    90 1F  -- 数据标识；
    01 -- 序列号；
    50 -- 校验码；
    16 -- 帧结束符；
************************************************************************/
static int32_t Packet01H(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t DataId, uint8_t DevType)
{
	uint16_t             usLen = 0; 
	uint8_t              i;
    LPCJT188ReadREG01H   pReadReg;
    if(Address > 0x0000FFFFFFFFFFFFLL)
    {
        return -1;
    }
	for(i = 0; i<LeadByteNum;i++)
	{
		*(pBuff+i) = 0xfe;
	}
    pReadReg = (LPCJT188ReadREG01H)(pBuff + LeadByteNum);

    pReadReg->StartFlag  = _FC_CJT188_START_FLAG;
    pReadReg->DevType = DevType;
    SetAddr(pReadReg->Addr, Address);
    pReadReg->CtlCode      = _FC_CJT188_READ_DATA;
    pReadReg->DataLen      = CJT188_READDATA_LEN;
    SetDataId(pReadReg->DataId, DataId);     
    pReadReg->SER = 1;
    pReadReg->CS = CalcSum(&pReadReg->StartFlag, pReadReg->DataLen + CJT188_COMMONFORM_SIZE);//CRC
    pReadReg->EndSymbol = _FC_CJT188_END_FLAG;//结束符
    usLen = CJT188_PACK_SIZE(pReadReg->DataLen);

	return usLen + LeadByteNum;
}



/************************************************************************
函数功能：UnPacket01H()
.  CJT188包帧应答标准格式
·　帧格式：| 帧起始符 | 仪表类型 | 地址域 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符 |        
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet01H 
        FE FE FE 68 10 03 00 01 20 00 00 00 81 16 90 1F 01 00 46 01 00 2C 00 46 01 00 2C 00 00 00 00 00 00 00 00 FF C8 16 
        FE FE FE --引导字符；
        68 -- 帧起始符；
        10 -- 表计类型代码；
        03 00 01 20 00 00 00 -- 表计地址；
        81 -- 控制码；
        16 -- 数据域长度；
        90 1F  -- 数据标识；
        01 -- 序列号；
        00 46 01 00 -- 当前累计流量；
        2C -- 单位：吨；
        00 46 01 00 -- 剩余流量；
        2C -- 单位：吨
        00 00 00 00 00 00 00 -- 实时时间；
        00 FF -- 表计状态；
        C8 -- 校验码；
        16 -- 帧结束符；
************************************************************************/
static int32_t UnPacket01H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t				 	headlen;
	uint8_t nReg,i,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_CJT188_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPCJT188ReadREG01H_ack  pReadDataAck;
    pReadDataAck = (LPCJT188ReadREG01H_ack) (pBuff + LeadByteNum);

    if(pReadDataAck->CtlCode != _FC_CJT188_READ_DATA_ACK_OK)
        return -1;
    wTpEndFlg = (uint8_t)(*(&pReadDataAck->StartFlag + CJT188_CS_COUNT_SIZE(pReadDataAck->DataLen) + CJT188_CRC_SIZE));

    if(wTpEndFlg != _FC_CJT188_END_FLAG)
    {
        return -1;
    }
	headlen = sizeof(struct tagReadREG01H_ack) + (pReadDataAck->DataLen);

    wTpCrc = (uint8_t)(*(&pReadDataAck->StartFlag + CJT188_CS_COUNT_SIZE(pReadDataAck->DataLen)));
    wCalCrc = CalcSum(&pReadDataAck->StartFlag, CJT188_CS_COUNT_SIZE(pReadDataAck->DataLen));

    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pReadDataAck->DataLen;

    if(nReg > 120)
    {
        return -1;
    }


    for(i=0; i< nReg; i++)
    {
        *(pVal+ i) = (uint8_t)(*(&pReadDataAck->StartFlag + CJT188_COMMONFORM_SIZE + 3 + i));
    }

    return nReg;    
}


const struct cjt188_encode_decode mb_encode_decode = {
	.Packet01H		= Packet01H 	 ,
	.UnPacket01H	= UnPacket01H	 ,
};

