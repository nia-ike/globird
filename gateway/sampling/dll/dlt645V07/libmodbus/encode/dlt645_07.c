
#include "dlt645_07.h"
#include "crc16.h"
#include <math.h>


#define _DEBUG      DEBUG_OFF

#define DLT645_07_PACK_SIZE(DataLen)		((DataLen) + DLT645_07_COMMONFORM_SIZE + DLT645_07_CRC_SIZE + DLT645_07_ENDFLAG_SIZE)	
#define DLT645_07_CS_COUNT_SIZE(DataLen)   ((DataLen) + DLT645_07_COMMONFORM_SIZE )	

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
    for(i=0; i<DLT645_07_COMM_ADDR_SIZE; i++)
    {
        temp = wAddr%100;
        pAddr[i] = ((temp/10)<<4) + ((temp%10) & 0x0F);
        wAddr /= 100;
    }   
    return 0;
}

///************************************************************************
//函数功能：      SetData()
//功能描述:       数据标识
//输入参数:       
//输出参数:       
//返 回 值:       
//说    明:       
//************************************************************************/
//  旧的是  BCD码方式配置。存在缺陷，不再使用。对应配置文件都有10进制数表示, 使用下面新函数
//int32_t SetData(uint8_t *pData, uint32_t wData, uint8_t num)
//{
//    uint8_t i;
//    uint32_t temp;
//    for( i = 0 ; i < num ; i++ )
//    {
//        temp = wData % 100;
//        pData[i] = ((temp/10)<<4) + ((temp%10) & 0x0F) + 0x33;
//        wData /= 100;
//    }
//    return 0;
//}


/************************************************************************
函数功能：      SetData()
功能描述:       数据标识
输入参数:       
输出参数:       
返 回 值:       
说    明:       
************************************************************************/
int32_t SetData(uint8_t *pData, uint32_t wData, uint8_t num)
{
    uint8_t i;
    uint32_t temp;
    for( i = 0 ; i < num ; i++ )
    {
        temp = wData & 0xff;
        pData[i] = temp + 0x33;
        wData >>= 8;
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
    LPDLT645_07ReadREG11H  pDLT645ReadData;
    pDLT645ReadData = (LPDLT645_07ReadREG11H )(pBuff) ;
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
函数功能：   DLT645Packet11H()
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
static int32_t Packet11H(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t DataId)
{
	uint16_t             usLen = 0; 
	uint8_t              i;
    LPDLT645_07ReadREG11H  pDLT645ReadReg;
    if(Address > 0x0000FFFFFFFFFFFFLL)
    {
        return -1;
    }
	for(i = 0; i<LeadByteNum;i++)
	{
		*(pBuff+i) = 0xfe;
	}
    pDLT645ReadReg = (LPDLT645_07ReadREG11H)(pBuff + LeadByteNum);

    pDLT645ReadReg->StartFlag_1  = _FC_DLT645_07_START_FLAG;
    SetAddr(pDLT645ReadReg->Addr, Address);
    pDLT645ReadReg->StartFlag_2  = _FC_DLT645_07_START_FLAG;
    pDLT645ReadReg->CtlCode      = _FC_DLT645_07_READ_DATA;
    pDLT645ReadReg->DataLen      = sizeof(pDLT645ReadReg->DataId);
    SetData(pDLT645ReadReg->DataId, DataId,DLT645_07_DATE_ID_SIZE);     
    pDLT645ReadReg->CS = CalcSum(&pDLT645ReadReg->StartFlag_1, pDLT645ReadReg->DataLen + DLT645_07_COMMONFORM_SIZE);//CRC
    pDLT645ReadReg->EndSymbol = _FC_DLT645_07_END_FLAG;//结束符
    usLen = DLT645_07_PACK_SIZE(pDLT645ReadReg->DataLen);

	return usLen + LeadByteNum;
}



/************************************************************************
函数功能：UnPacket91H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet11H 
************************************************************************/
static int32_t UnPacket91H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,i,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07ReadREG91H_ack  pDLT645ReadDataAck;
    pDLT645ReadDataAck = (LPDLT645_07ReadREG91H_ack) (pBuff + LeadByteNum);
    if(pDLT645ReadDataAck->CtlCode == _FC_DLT645_07_READ_DATA_ACK_ERR)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }

    wTpCrc = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645ReadDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645ReadDataAck->DataLen - 4;
    if(nReg > 120)
    {
        return -1;
    }
    for(i=0; i< nReg; i++)
    {
        *(pVal+ i) = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_COMMONFORM_SIZE + 4 + i)) - 0x33;
    }

    return nReg;    
}

/************************************************************************
函数功能：UnPacketD1H()
               
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet11H 
************************************************************************/
static int32_t UnPacketD1H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07ReadREGD1H_ack  pDLT645ReadDataAck;
    pDLT645ReadDataAck = (LPDLT645_07ReadREGD1H_ack) (pBuff + LeadByteNum);
    if(pDLT645ReadDataAck->CtlCode != _FC_DLT645_07_READ_DATA_ACK_ERR)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645ReadDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645ReadDataAck->DataLen;
    if(nReg != 1)
    {
        return -1;
    }
    *pVal= MB_ALIGN_PT_8BIT_READ((pBuff + sizeof(struct tagReadREGD1H_ack))) - 0x33;

    return nReg;    
}
/************************************************************************
函数功能：   DLT645Packet13H()
功能描述:       读数据功能码处理
输入参数:        pData，数据标识
                pAckMsg，请求帧
                DataLen，数据域长度；
输出参数:        pAckMsg，请求帧的指针
返 回 值:       返回报文的长度
说    明:       读数据

.  DLT645包帧标准格式
·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 校验码 | 结束符
·　DLT645读数据的包格式如下：
·　功能：读通讯地址
·　控制码：C=13H
·　数据域长度：L=04H+m（数据长度）

************************************************************************/
static int32_t Packet13H(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address)
{
	uint16_t             usLen = 0; 
	uint8_t              i;
    LPDLT645_07ReadREG13H  pDLT645ReadReg;
    if(Address > 0x0000FFFFFFFFFFFFLL)
    {
        return -1;
    }
	for(i = 0; i<LeadByteNum;i++)
	{
		*(pBuff+i) = 0xfe;
	}
    pDLT645ReadReg = (LPDLT645_07ReadREG13H)(pBuff + LeadByteNum);

    pDLT645ReadReg->StartFlag_1  = _FC_DLT645_07_START_FLAG;
	for(int n = 0; n<DLT645_07_COMM_ADDR_SIZE;n++)
	{
        pDLT645ReadReg->Addr[n] = 0xAA;    
	}
    pDLT645ReadReg->StartFlag_2  = _FC_DLT645_07_START_FLAG;
    pDLT645ReadReg->CtlCode      = _FC_DLT645_07_READ_COMMADDR;
    pDLT645ReadReg->DataLen      = 0;

    pDLT645ReadReg->CS = CalcSum(&pDLT645ReadReg->StartFlag_1, pDLT645ReadReg->DataLen + DLT645_07_COMMONFORM_SIZE);//CRC

    pDLT645ReadReg->EndSymbol = _FC_DLT645_07_END_FLAG;//结束符
    usLen = DLT645_07_PACK_SIZE(pDLT645ReadReg->DataLen);

	return usLen + LeadByteNum;
}


/************************************************************************
函数功能：UnPacket91H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet11H 
************************************************************************/
static int32_t UnPacket93H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,i,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07ReadREG93H_ack  pDLT645ReadDataAck;
    pDLT645ReadDataAck = (LPDLT645_07ReadREG93H_ack) (pBuff + LeadByteNum);
    if(pDLT645ReadDataAck->CtlCode != _FC_DLT645_07_READ_COMMADDR_ACK_OK)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645ReadDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645ReadDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645ReadDataAck->DataLen;
    if(nReg != 6)
    {
        return -1;
    }
    for(i=0; i< nReg; i++)
    {
        *(pVal+ i) = (uint8_t)(*(&pDLT645ReadDataAck->StartFlag_1 + DLT645_07_COMMONFORM_SIZE + i)) - 0x33;
    }
    return nReg;    
}


/************************************************************************
函数功能：   DLT645Packet14H()
功能描述:       写数据功能码处理
输入参数:        pData，数据标识
                pAckMsg，请求帧
                DataLen，数据域长度；
                LeadByteNum，前导符个数
                Address，地址
                DataId，操作者代码

输出参数:        pAckMsg，请求帧的指针
返 回 值:       返回报文的长度
说    明:       写数据

.  DLT645包帧标准格式
·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符
·　DLT645写数据的包格式如下：
·　功能：主站向从站请求设置数据 ( 或编程 )
·　控制码：C=14H
·　数据域长度：L=04H+04H（密码）+04H（操作者代码）+m（数据长度）
·　数据域： DI0DI1DI2DI3+PAP0P1P2+C0C1C2C3+DATA
·　注1：PA表示该密码权限，P0P1P2为密码
·　注2：C0C1C2C3是操作者代码，为要求记录操作人员信息的项目提供数据
·　注3：本命令必须与编程键配合使用
************************************************************************/
static int32_t Packet14H(uint8_t *pBuff, uint16_t LeadByteNum, uint64_t Address, uint32_t DataId, uint32_t password, uint32_t operator,  uint16_t *pVal, uint16_t wRegNum)
{ 
    uint8_t *CS,*EndSymbol;
	uint16_t             usLen = 0; 
	uint8_t              i;

    LPDLT645_07WriteREG14H  pDLT645WriteReg;
    if(Address > 0x0000FFFFFFFFFFFFLL)
    {
        return -1;
    }
	for(i = 0; i<LeadByteNum;i++)
	{
		*(pBuff+i) = 0xfe;
	}

    pDLT645WriteReg = (LPDLT645_07WriteREG14H)(pBuff + LeadByteNum);

    pDLT645WriteReg->StartFlag_1  = _FC_DLT645_07_START_FLAG;
    SetAddr(pDLT645WriteReg->Addr, Address);
    pDLT645WriteReg->StartFlag_2  = _FC_DLT645_07_START_FLAG;
    pDLT645WriteReg->CtlCode      = _FC_DLT645_07_WRITE_DATA;
    pDLT645WriteReg->DataLen      = DLT645_07_DATE_ID_SIZE+ DLT645_07_W_PASSWORD_SIZE + DLT645_07_C_EVENT_ID_SIZE + wRegNum;
    SetData(pDLT645WriteReg->DataId, DataId,DLT645_07_DATE_ID_SIZE);     
    SetData(pDLT645WriteReg->Password, password,DLT645_07_W_PASSWORD_SIZE);     
    SetData(pDLT645WriteReg->Event, operator,DLT645_07_C_EVENT_ID_SIZE);     

    SetData(&pDLT645WriteReg->pFrameData, *pVal, wRegNum);     

    CS = (pBuff + LeadByteNum + DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen));
    *CS = CalcSum(&pDLT645WriteReg->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen));
    EndSymbol = (pBuff + LeadByteNum + DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen) +1);
    *EndSymbol = _FC_DLT645_07_END_FLAG;//结束符
    usLen = DLT645_07_PACK_SIZE(pDLT645WriteReg->DataLen);

	return usLen + LeadByteNum;
}

/************************************************************************
函数功能：UnPacket94H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet14H 正常应答帧
************************************************************************/
static int32_t UnPacket94H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07WriteREG94H_ack  pDLT645WriteDataAck;
    pDLT645WriteDataAck = (LPDLT645_07WriteREG94H_ack) (pBuff + LeadByteNum);
    if(pDLT645WriteDataAck->CtlCode != _FC_DLT645_07_WRITE_DATA_ACK_OK)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645WriteDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645WriteDataAck->DataLen;
    if(nReg != 0)
    {
        return -1;
    }
    return nReg;    
}

/************************************************************************
函数功能：UnPacketD4H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet14H 异常应答帧
************************************************************************/
static int32_t UnPacketD4H(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,i,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07WriteREGD4H_ack  pDLT645WriteDataAck;
    pDLT645WriteDataAck = (LPDLT645_07WriteREGD4H_ack) (pBuff + LeadByteNum);
    if(pDLT645WriteDataAck->CtlCode != _FC_DLT645_07_WRITE_DATA_ACK_ERR)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645WriteDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645WriteDataAck->DataLen;
    if(nReg != 1)
    {
        return -1;
    }
    for(i=0; i< nReg; i++)
    {
        *(pVal+ i) = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_COMMONFORM_SIZE + i)) - 0x33;
    }
    return nReg;    
}



/************************************************************************
函数功能：   Packet1CH()
功能描述:       写数据功能码处理
输入参数:        pData，数据标识
                pAckMsg，请求帧
                DataLen，数据域长度；
                LeadByteNum，前导符个数
                Address，地址
                DataId，操作者代码

输出参数:        pAckMsg，请求帧的指针
返 回 值:       返回报文的长度
说    明:       写数据

.  DLT645包帧标准格式
·　帧格式：| 帧起始符 | 地址域 | 帧起始符 | 控制码 | 数据域长度 | 数据域 | 校验码 | 结束符
·　DLT645写数据的包格式如下：
·　功能：主站向从站请求设置数据 ( 或编程 )
·　控制码：C=14H
·　数据域长度：L=04H+04H（密码）+04H（操作者代码）+m（数据长度）
·　数据域： DI0DI1DI2DI3+PAP0P1P2+C0C1C2C3+DATA
·　注1：PA表示该密码权限，P0P1P2为密码
·　注2：C0C1C2C3是操作者代码，为要求记录操作人员信息的项目提供数据
·　注3：本命令必须与编程键配合使用
************************************************************************/
static int32_t Packet1CH(uint8_t *pBuff, uint16_t LeadByteNum, uint64_t Address, uint32_t DataId, uint32_t password, uint32_t operator,  uint16_t *pVal, uint16_t wRegNum)
{ 
    uint8_t *CS,*EndSymbol;
	uint16_t             usLen = 0; 
	uint8_t              i;

    LPDLT645_07WriteREG14H  pDLT645WriteReg;
    if(Address > 0x0000FFFFFFFFFFFFLL)
    {
        return -1;
    }
	for(i = 0; i<LeadByteNum;i++)
	{
		*(pBuff+i) = 0xfe;
	}

    pDLT645WriteReg = (LPDLT645_07WriteREG14H)(pBuff + LeadByteNum);

    pDLT645WriteReg->StartFlag_1  = _FC_DLT645_07_START_FLAG;
    SetAddr(pDLT645WriteReg->Addr, Address);
    pDLT645WriteReg->StartFlag_2  = _FC_DLT645_07_START_FLAG;
    pDLT645WriteReg->CtlCode      = _FC_DLT645_07_WRITE_CONTROL;
    pDLT645WriteReg->DataLen      = DLT645_07_DATE_ID_SIZE+ DLT645_07_W_PASSWORD_SIZE + DLT645_07_C_EVENT_ID_SIZE + wRegNum;
    SetData(pDLT645WriteReg->DataId, DataId,DLT645_07_DATE_ID_SIZE);     
    SetData(pDLT645WriteReg->Password, password,DLT645_07_W_PASSWORD_SIZE);     
    SetData(pDLT645WriteReg->Event, operator,DLT645_07_C_EVENT_ID_SIZE);     

    SetData(&pDLT645WriteReg->pFrameData, *pVal, wRegNum);     

    CS = (pBuff + LeadByteNum + DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen));
    *CS = CalcSum(&pDLT645WriteReg->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen));
    EndSymbol = (pBuff + LeadByteNum + DLT645_07_CS_COUNT_SIZE(pDLT645WriteReg->DataLen) +1);
    *EndSymbol = _FC_DLT645_07_END_FLAG;//结束符
    usLen = DLT645_07_PACK_SIZE(pDLT645WriteReg->DataLen);

	return usLen + LeadByteNum;
}

/************************************************************************
UnPacket9CH()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet14H 正常应答帧
************************************************************************/
static int32_t UnPacket9CH(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07WriteREG94H_ack  pDLT645WriteDataAck;
    pDLT645WriteDataAck = (LPDLT645_07WriteREG94H_ack) (pBuff + LeadByteNum);
    if(pDLT645WriteDataAck->CtlCode != _FC_DLT645_07_WRITE_CONTROL_ACK_OK)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645WriteDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645WriteDataAck->DataLen;
    if(nReg != 0)
    {
        return -1;
    }
    return nReg;    
}

/************************************************************************
函数功能：UnPacketDCH()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：Packet14H 异常应答帧
************************************************************************/
static int32_t UnPacketDCH(uint8_t *pBuff, uint8_t *pVal)
{
	uint8_t nReg,i,LeadByteNum = 0;
    uint8_t wTpCrc, wCalCrc,wTpEndFlg;

    while((*(pBuff + LeadByteNum) != _FC_DLT645_07_START_FLAG) && (LeadByteNum < 200))
    {
        LeadByteNum ++;
    }
    if(LeadByteNum == 200)
    {
        return -1;
    }

    LPDLT645_07WriteREGD4H_ack  pDLT645WriteDataAck;
    pDLT645WriteDataAck = (LPDLT645_07WriteREGD4H_ack) (pBuff + LeadByteNum);
    if(pDLT645WriteDataAck->CtlCode != _FC_DLT645_07_WRITE_CONTROL_ACK_ERR)
        return -1;

    wTpEndFlg = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen) + DLT645_07_CRC_SIZE));
    if(wTpEndFlg != _FC_DLT645_07_END_FLAG)
    {
        return -1;
    }
    wTpCrc = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen)));
    wCalCrc = CalcSum(&pDLT645WriteDataAck->StartFlag_1, DLT645_07_CS_COUNT_SIZE(pDLT645WriteDataAck->DataLen));
    if(wTpCrc != wCalCrc)
    {
        return -1;
    }

    nReg = pDLT645WriteDataAck->DataLen;
    if(nReg != 1)
    {
        return -1;
    }
    for(i=0; i< nReg; i++)
    {
        *(pVal+ i) = (uint8_t)(*(&pDLT645WriteDataAck->StartFlag_1 + DLT645_07_COMMONFORM_SIZE + i)) - 0x33;
    }
    return nReg;    
}

const struct dlt645_07_encode_decode mb_encode_decode = {
	.Packet11H		= Packet11H 	 ,
	.UnPacket91H	= UnPacket91H	 ,
	.UnPacketD1H	= UnPacketD1H	 ,
	.Packet13H		= Packet13H 	 ,
	.UnPacket93H	= UnPacket93H	 ,
 	.Packet14H		= Packet14H 	 ,
	.UnPacket94H	= UnPacket94H	 ,
	.UnPacketD4H	= UnPacketD4H	 ,
    .Packet1CH		= Packet1CH 	 ,
	.UnPacket9CH	= UnPacket9CH	 ,
};

