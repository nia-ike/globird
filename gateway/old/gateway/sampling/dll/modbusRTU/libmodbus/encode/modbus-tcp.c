
#include "modbus-rtu.h"
#include "crc16.h"


#define _DEBUG      DEBUG_OFF



static uint16_t m_wTransmitFlag = 0;
static uint16_t m_wPotocolFlag = 0;

/************************************************************************
函数功能：ModbusPacket01H()
            MODBUS 读继电器组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wRegNum  寄存器数量
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet01H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum)
{
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;
    LPModBusReadRly01H   pReadRly01H;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadRly01H = (LPModBusReadRly01H)(pBuff + sizeof(struct tagMBAPHead));
    pReadRly01H->SlaveAddr  = Address;
    pReadRly01H->Cmd        = _FC_READ_COILS;
    pReadRly01H->wRegAddr   = SWAPBYTE(wRegAddr);
    pReadRly01H->wRegNum    = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagReadRly01H);
   
    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);
        
    return headlen;
}

/************************************************************************
函数功能：ModbusPacket02H()
            MODBUS 读开关量组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wRegNum  寄存器数量
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet02H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum)
{ 
    LPModBusReadDI02H    pReadDI;
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadDI = (LPModBusReadDI02H)(pBuff + sizeof(struct tagMBAPHead));

    pReadDI->SlaveAddr      = Address;
    pReadDI->Cmd            = _FC_READ_DISCRETE_INPUTS;
    pReadDI->wRegAddr       = SWAPBYTE(wRegAddr);
    pReadDI->wRegNum        = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagReadDI02H);

    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);

    return headlen;
}

/************************************************************************
函数功能：ModbusPacket03H()
            MODBUS 读保持寄存器组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wRegNum  寄存器数量
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet03H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum)
{ 
    LPModBusReadREG03H  pReadReg;
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadReg = (LPModBusReadREG03H)(pBuff + sizeof(struct tagMBAPHead));

    pReadReg->SlaveAddr = Address;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->wRegAddr   = SWAPBYTE(wRegAddr);
    pReadReg->wRegNum    = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagReadREG03H);

    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);

    return headlen;
}


/************************************************************************
函数功能：ModbusPacket04H()
            MODBUS 读输入寄存器组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wRegNum  寄存器数量
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet04H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum)
{ 
    LPModBusReadREG04H  pReadReg;
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;
    
    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadReg = (LPModBusReadREG04H)(pBuff + sizeof(struct tagMBAPHead));

    pReadReg->SlaveAddr     = Address;
    pReadReg->Cmd           = _FC_READ_INPUT_REGISTERS;
    pReadReg->wRegAddr      = SWAPBYTE(wRegAddr);
    pReadReg->wRegNum       = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagReadREG04H);

    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);

    return headlen;
}



/************************************************************************
函数功能：Packet05H()
            MODBUS 写继电器组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wOpenClose 开/合           0:开 1:合
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet05H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wOpenClose)
{
    LPModBusWriteRly05H pWriteRly;
    uint16_t            optValue = 0;
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pWriteRly = (LPModBusWriteRly05H)(pBuff + sizeof(struct tagMBAPHead));

    pWriteRly->SlaveAddr    = Address;
    pWriteRly->Cmd          = _FC_WRITE_SINGLE_COIL;
    pWriteRly->RlyAddr      = SWAPBYTE(wRegAddr);
    //modify by Wangcq
    if(wOpenClose != 0)
    {
        optValue = 0x00FF;
    }
    else
    {
        optValue = 0x0;
    }
    
    pWriteRly->RlyVal   = optValue;

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagWriteRly05H);

    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);

    return headlen;
}


/************************************************************************
函数功能：Packet06H()
            MODBUS 写单保持寄存器（模拟量输出）组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            OnOff   0 - 断开  1 - 闭合
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet06H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wValue,uint8_t Attr,uint16_t Password)
{
	uint8_t				 headlen;
    LPModbusMBAPHead     pMBAPHead;
	
    pMBAPHead   = (LPModbusMBAPHead) pBuff;

    if(Attr)
    {
        //需要先写密码的
        LPModBusWriteSysReg06H pWriteSysReg;

        pWriteSysReg = (LPModBusWriteSysReg06H)(pBuff + sizeof(struct tagMBAPHead));

        pWriteSysReg->SlaveAddr    = Address;
        pWriteSysReg->Cmd          = _FC_WRITE_SINGLE_REGISTER;
        pWriteSysReg->RegAddr      = SWAPBYTE(wRegAddr);
        pWriteSysReg->Password     = SWAPBYTE(Password);
        pWriteSysReg->RegVal       = SWAPBYTE(wValue);

		headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagWriteReg06H);

        // Modbus MBAP head
        pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
        pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
        pMBAPHead->Len          = SWAP_16BIT(headlen);

        return headlen;
    }
    else
    {
        LPModBusWriteReg06H pWriteReg;

        pWriteReg = (LPModBusWriteReg06H)(pBuff + sizeof(struct tagMBAPHead));

        pWriteReg->SlaveAddr    = Address;
        pWriteReg->Cmd          = _FC_WRITE_SINGLE_REGISTER;
        pWriteReg->RegAddr      = SWAPBYTE(wRegAddr);
        pWriteReg->RegVal       = SWAPBYTE(wValue);

		headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagWriteReg06H);

        // Modbus MBAP head
        pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
        pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
        pMBAPHead->Len          = SWAP_16BIT(headlen);

        return headlen;
    }
}

/************************************************************************
函数功能：AddReg10H()
            MODBUS 写寄存器组包添加寄存器
输入参数：
输出参数：  
返回值  ：
说明    ：
************************************************************************/
static int32_t AddReg10H(LPModBusWriteREG10H pWriteReg, uint16_t RegVal)
{
    uint16_t swap_regVal = SWAPBYTE(RegVal);
    MB_ALIGN_PT_16BIT_WRITE(&pWriteReg->SlaveAddr + sizeof(struct tagWriteREG10H) + pWriteReg->BytesNum, swap_regVal);
    pWriteReg->BytesNum = pWriteReg->BytesNum + REG_SIZE;
    pWriteReg->wRegNum   = SWAPBYTE(pWriteReg->BytesNum / REG_SIZE);
    if(pWriteReg->BytesNum > 200)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}
/************************************************************************
函数功能：ModbusPacket10H()
            MODBUS 写寄存器组包
输入参数：  pBuff    组包缓冲区（一般设置缓冲不大于300）
            Address 从机地址
            wRegAddr 寄存器其实地址
            wRegNum  寄存器数量
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t Packet10H(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password)
{
    int32_t              i;
    LPModBusWriteREG10H pWriteReg;
    LPModbusMBAPHead     pMBAPHead;
	uint8_t				 headlen;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pWriteReg = (LPModBusWriteREG10H)(pBuff + sizeof(struct tagMBAPHead));

    pWriteReg->SlaveAddr        = Address;
    pWriteReg->Cmd              = _FC_WRITE_MULTIPLE_REGISTERS;
    pWriteReg->wRegAddr         = SWAPBYTE(wRegAddr);

    /* 初始写寄存器数量为0，在下面AddReg()后自动添加 */
    pWriteReg->wRegNum          = 0;
    pWriteReg->BytesNum         = 0;

    if(Attr)
    {
        //需要写密码
        AddReg10H(pWriteReg, Password);
    }
    for(i=0;i<wRegNum;i++)
    {
        AddReg10H(pWriteReg, *(pVal+i));
    }

	headlen = sizeof(struct tagMBAPHead) + sizeof(struct tagWriteREG10H) + pWriteReg->BytesNum;

    // Modbus MBAP head
    pMBAPHead->TransmitFlag = m_wTransmitFlag;                          /* Transaction ID */
    pMBAPHead->PotocolFlag  = m_wPotocolFlag;                           /* Protocol Modbus */
    pMBAPHead->Len          = SWAP_16BIT(headlen);

    return headlen;
}




/************************************************************************
函数功能：UnPacket01H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket01H(uint8_t *pBuff, uint16_t *pVal)
{
    int32_t                 i;
    LPModBusReadRly01H_ack  pReadRlyAck;
    LPModbusMBAPHead        pMBAPHead;
	uint8_t				 	headlen;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadRlyAck = (LPModBusReadRly01H_ack) (pBuff + sizeof(struct tagMBAPHead));
    
    if(pReadRlyAck->Cmd != _FC_READ_COILS)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

	headlen = sizeof(struct tagReadRly01H_ack);
    
    for(i=0;i<pReadRlyAck->ByteNum;i++)
    {
        *(pVal+i) = *((pBuff + headlen) + i);;
    }
    return pReadRlyAck->ByteNum;
    
}

/************************************************************************
函数功能：UnPacket02H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket02H(uint8_t *pBuff, uint16_t *pVal)
{
    int32_t                 i;
    LPModBusReadDI02H_ack   pReadDIAck;
    LPModbusMBAPHead        pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadDIAck = (LPModBusReadDI02H_ack) (pBuff + sizeof(struct tagMBAPHead));

    if(pReadDIAck->Cmd != _FC_READ_DISCRETE_INPUTS)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    
    for(i=0; i<pReadDIAck->ByteNum; i++)
    {
        *(pVal+i) = *((pBuff + sizeof(struct tagReadDI02H_ack)) + i);
    }
    return pReadDIAck->ByteNum;
    
}

/************************************************************************
函数功能：UnPacket03H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket03H(uint8_t *pBuff, uint16_t *pVal)
{
    uint32_t                i=0;
    uint8_t                 nReg;
    LPModBusReadREG03H_ack  pReadRegAck;
    LPModbusMBAPHead        pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadRegAck = (LPModBusReadREG03H_ack) (pBuff + sizeof(struct tagMBAPHead));
    if(pReadRegAck->Cmd != _FC_READ_HOLDING_REGISTERS)
    {
        return -1;
    }
    
    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    nReg = pReadRegAck->nbytes / REG_SIZE;

    if(nReg > 125)
    {
        return -1;
    }

    for(i=0; i<nReg; i++)
    {
        if(pVal)
        {
            *(pVal+ i) = MB_ALIGN_PT_16BIT_READ((pBuff + sizeof(struct tagReadREG03H_ack)) + 2*i);
        }
    }
    return i;
  
}


/************************************************************************
函数功能：UnPacket04H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket04H(uint8_t *pBuff, uint16_t *pVal)
{
    uint32_t                i;
    uint8_t                 nReg;
    LPModBusReadREG04H_ack  pReadRegAck;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pReadRegAck = (LPModBusReadREG04H_ack) (pBuff + sizeof(struct tagMBAPHead));

    if(pReadRegAck->Cmd != _FC_READ_INPUT_REGISTERS)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    nReg = pReadRegAck->nbytes / REG_SIZE;
    if(nReg > 125)
    {
        return -1;
    }

    for(i=0; i<nReg; i++)
    {
        *(pVal+ i) = MB_ALIGN_PT_16BIT_READ((pBuff + sizeof(struct tagReadREG03H_ack)) + 2*i);
    }
    return i;
   
}

/************************************************************************
函数功能：UnPacket05H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket05H(uint8_t *pBuff)
{
    LPModBusWriteRly05H_ack pWriteRlyAck;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pWriteRlyAck = (LPModBusWriteRly05H_ack) (pBuff + sizeof(struct tagMBAPHead));

    if(pWriteRlyAck->Cmd != _FC_WRITE_SINGLE_COIL)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    return 0;
}

/************************************************************************
函数功能：UnPacket06H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket06H(uint8_t *pBuff)
{
    LPModBusWriteReg06H_ack pWriteRegAck;
    LPModbusMBAPHead     pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pWriteRegAck = (LPModBusWriteReg06H_ack) (pBuff + sizeof(struct tagMBAPHead));

    if(pWriteRegAck->Cmd != _FC_WRITE_SINGLE_REGISTER)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    return 0;
}

/************************************************************************
函数功能：UnPacket05H()
            
输入参数：  
输出参数：  
返回值  ：数据包大小
说明    ：
************************************************************************/
static int32_t UnPacket10H(uint8_t *pBuff)
{
    LPModBusWriteREG10H_ack pWriteRegAck;
    LPModbusMBAPHead        pMBAPHead;

    pMBAPHead   = (LPModbusMBAPHead) pBuff;
    pWriteRegAck = (LPModBusWriteREG10H_ack) (pBuff + sizeof(struct tagMBAPHead));

    if(pWriteRegAck->Cmd != _FC_WRITE_MULTIPLE_REGISTERS)
    {
        return -1;
    }

    m_wTransmitFlag = pMBAPHead->TransmitFlag;                          /* Transaction ID */
    m_wPotocolFlag  = pMBAPHead->PotocolFlag;                           /* Protocol Modbus */

    return 0;
}



const struct modbus_encode_decode mb_tcp_encode_decode = {
	.Packet01H		= Packet01H 	 ,
	.Packet02H		= Packet02H 	 ,
	.Packet03H		= Packet03H 	 ,
	.Packet04H		= Packet04H 	 ,
	.Packet05H		= Packet05H 	 ,
	.Packet06H		= Packet06H 	 ,
	.Packet10H		= Packet10H 	 ,
	.UnPacket01H	= UnPacket01H	 ,
	.UnPacket02H	= UnPacket02H	 ,
	.UnPacket03H	= UnPacket03H	 ,
	.UnPacket04H	= UnPacket04H	 ,
	.UnPacket05H	= UnPacket05H	 ,
	.UnPacket06H	= UnPacket06H	 ,
	.UnPacket10H	= UnPacket10H	 
	
};

