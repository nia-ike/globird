
#include "modbus-rtu.h"
#include "crc16.h"


#define _DEBUG      DEBUG_OFF


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
    uint16_t             Crc; 
	uint8_t				 headlen;
    LPModBusReadRly01H  pReadRly01H;

    pReadRly01H = (LPModBusReadRly01H) pBuff;
    pReadRly01H->SlaveAddr  = Address;
    pReadRly01H->Cmd        = _FC_READ_COILS;
    pReadRly01H->wRegAddr   = SWAPBYTE(wRegAddr);
    pReadRly01H->wRegNum    = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagReadRly01H);
    Crc = crc16(&pReadRly01H->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);
        
    return headlen + CRC16_SIZE;
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
    uint16_t             Crc; 
    LPModBusReadDI02H    pReadDI;
	uint8_t				 headlen;
	
    pReadDI = (LPModBusReadDI02H)pBuff;

    pReadDI->SlaveAddr      = Address;
    pReadDI->Cmd            = _FC_READ_DISCRETE_INPUTS;
    pReadDI->wRegAddr       = SWAPBYTE(wRegAddr);
    pReadDI->wRegNum        = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagReadDI02H);

    Crc = crc16(&pReadDI->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

    return headlen + CRC16_SIZE;
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
    uint16_t             Crc;  
    LPModBusReadREG03H  pReadReg;
	uint8_t				 headlen;
	
    pReadReg = (LPModBusReadREG03H)pBuff;

    pReadReg->SlaveAddr = Address;
    pReadReg->Cmd       = _FC_READ_HOLDING_REGISTERS;
    pReadReg->wRegAddr   = SWAPBYTE(wRegAddr);
    pReadReg->wRegNum    = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagReadREG03H);

    Crc = crc16(&pReadReg->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

    return headlen + CRC16_SIZE;
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
    uint16_t             Crc;  
    LPModBusReadREG04H  pReadReg;
	uint8_t				 headlen;

    pReadReg = (LPModBusReadREG04H)pBuff;

    pReadReg->SlaveAddr     = Address;
    pReadReg->Cmd           = _FC_READ_INPUT_REGISTERS;
    pReadReg->wRegAddr      = SWAPBYTE(wRegAddr);
    pReadReg->wRegNum       = SWAPBYTE(wRegNum);

	headlen = sizeof(struct tagReadREG04H);

    Crc = crc16(&pReadReg->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

    return headlen + CRC16_SIZE;
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
    uint16_t             Crc; 
    LPModBusWriteRly05H pWriteRly;
    uint16_t            optValue = 0;
	uint8_t				 headlen;

    pWriteRly = (LPModBusWriteRly05H)pBuff;

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

	headlen = sizeof(struct tagWriteRly05H);

    Crc = crc16(&pWriteRly->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

    return headlen + CRC16_SIZE;
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
    uint16_t             Crc; 
	uint8_t				 headlen;
	
    if(Attr)
    {
        //需要先写密码的
        LPModBusWriteSysReg06H pWriteSysReg;

        pWriteSysReg = (LPModBusWriteSysReg06H)pBuff;

        pWriteSysReg->SlaveAddr    = Address;
        pWriteSysReg->Cmd          = _FC_WRITE_SINGLE_REGISTER;
        pWriteSysReg->RegAddr      = SWAPBYTE(wRegAddr);
        pWriteSysReg->Password     = SWAPBYTE(Password);
        pWriteSysReg->RegVal       = SWAPBYTE(wValue);

		headlen = sizeof(struct tagWriteReg06H);

        Crc = crc16(&pWriteSysReg->SlaveAddr, headlen);
        Crc = SWAP_16BIT(Crc);
        MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

        return headlen + CRC16_SIZE;
    }
    else
    {
        LPModBusWriteReg06H pWriteReg;

        pWriteReg = (LPModBusWriteReg06H)pBuff;

        pWriteReg->SlaveAddr    = Address;
        pWriteReg->Cmd          = _FC_WRITE_SINGLE_REGISTER;
        pWriteReg->RegAddr      = SWAPBYTE(wRegAddr);
        pWriteReg->RegVal       = SWAPBYTE(wValue);

		headlen = sizeof(struct tagWriteReg06H);

        Crc = crc16(&pWriteReg->SlaveAddr, headlen);
        Crc = SWAP_16BIT(Crc);
        MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

        return headlen + CRC16_SIZE;
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
    int32_t             i;
    uint16_t             Crc; 
    LPModBusWriteREG10H pWriteReg;
	uint8_t				 headlen;

    pWriteReg = (LPModBusWriteREG10H)pBuff;

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

	headlen = sizeof(struct tagWriteREG10H) + pWriteReg->BytesNum;

    Crc = crc16(&pWriteReg->SlaveAddr, headlen);
    Crc = SWAP_16BIT(Crc);
    MB_ALIGN_PT_16BIT_WRITE(pBuff + headlen, Crc);

    return headlen + CRC16_SIZE;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusReadRly01H_ack  pReadRlyAck;
	uint8_t				 	headlen;

    pReadRlyAck = (LPModBusReadRly01H_ack) pBuff;
    
    if(pReadRlyAck->Cmd != _FC_READ_COILS)
    {
        return -1;
    }

	headlen = sizeof(struct tagReadRly01H_ack) + pReadRlyAck->ByteNum;
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadRlyAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pReadRlyAck->SlaveAddr, headlen);
    if(wTpCrc == wCalCrc)
    {
        for(i=0;i<pReadRlyAck->ByteNum;i++)
        {
            *(pVal+i) = *((pBuff + sizeof(struct tagReadRly01H_ack)) + i);
        }
        return pReadRlyAck->ByteNum;
    }
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusReadDI02H_ack   pReadDIAck;
	uint8_t				 	headlen;

    pReadDIAck = (LPModBusReadDI02H_ack) pBuff;

    if(pReadDIAck->Cmd != _FC_READ_DISCRETE_INPUTS)
    {
        return -1;
    }

	headlen = sizeof(struct tagReadDI02H_ack) + pReadDIAck->ByteNum;
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadDIAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pReadDIAck->SlaveAddr, headlen);

    if(wTpCrc == wCalCrc)
    {
        for(i=0; i<pReadDIAck->ByteNum; i++)
        {
            *(pVal+i) = *((pBuff + sizeof(struct tagReadDI02H_ack)) + i);
        }
        return pReadDIAck->ByteNum;
    }
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusReadREG03H_ack  pReadRegAck;
	uint8_t				 	headlen;

    pReadRegAck = (LPModBusReadREG03H_ack) pBuff;
    if(pReadRegAck->Cmd != _FC_READ_HOLDING_REGISTERS)
    {
        return -1;
    }

	headlen = sizeof(struct tagReadREG03H_ack) + (pReadRegAck->nbytes);
    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadRegAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pReadRegAck->SlaveAddr, headlen);
    if(wTpCrc == wCalCrc)
    {
        nReg = pReadRegAck->nbytes / REG_SIZE;

        if(nReg > 125)
            return -1;
        if(!pVal)
            return -1;

        for(i=0; i<nReg; i++)
        {
        	*(pVal+ i) = MB_ALIGN_PT_16BIT_READ((pBuff + sizeof(struct tagReadREG03H_ack)) + 2*i);
        }
        return i;
    }
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusReadREG04H_ack  pReadRegAck;
	uint8_t				 	headlen;

    pReadRegAck = (LPModBusReadREG04H_ack) pBuff;

    if(pReadRegAck->Cmd != _FC_READ_INPUT_REGISTERS)
    {
        return -1;
    }

	headlen = sizeof(struct tagReadREG04H_ack) + (pReadRegAck->nbytes);

    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pReadRegAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pReadRegAck->SlaveAddr, headlen);

    if(wTpCrc == wCalCrc)
    {
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
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusWriteRly05H_ack pWriteRlyAck;
	uint8_t				 	headlen;

    pWriteRlyAck = (LPModBusWriteRly05H_ack) pBuff;

    if(pWriteRlyAck->Cmd != _FC_WRITE_SINGLE_COIL)
    {
        return -1;
    }

	headlen = sizeof(struct tagWriteRly05H_ack);

    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteRlyAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pWriteRlyAck->SlaveAddr, headlen);

    if(wTpCrc == wCalCrc)
    {
        return 0;
    }
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusWriteReg06H_ack pWriteRegAck;
	uint8_t				 	headlen;

    pWriteRegAck = (LPModBusWriteReg06H_ack) pBuff;

    if(pWriteRegAck->Cmd != _FC_WRITE_SINGLE_REGISTER)
    {
        return -1;
    }

	headlen = sizeof(struct tagWriteReg06H_ack);

    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteRegAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pWriteRegAck->SlaveAddr, headlen);

    if(wTpCrc == wCalCrc)
    {
        return 0;
    }
    return -1;
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
    uint16_t                wTpCrc, wCalCrc;
    LPModBusWriteREG10H_ack pWriteRegAck;
	uint8_t				 	headlen;

    pWriteRegAck = (LPModBusWriteREG10H_ack) pBuff;

    if(pWriteRegAck->Cmd != _FC_WRITE_MULTIPLE_REGISTERS)
    {
        return -1;
    }

	headlen = sizeof(struct tagWriteREG10H_ack);

    wTpCrc = MB_ALIGN_PT_16BIT_READ(&pWriteRegAck->SlaveAddr + headlen);
    wTpCrc = SWAP_16BIT(wTpCrc);
    wCalCrc = crc16(&pWriteRegAck->SlaveAddr, headlen);

    if(wTpCrc == wCalCrc)
    {
        return 0;
    }
    return -1;
}



const struct modbus_encode_decode mb_encode_decode = {
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

