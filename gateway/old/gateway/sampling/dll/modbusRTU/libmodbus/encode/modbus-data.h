#ifndef _MODBUS_DATA_H_
#define _MODBUS_DATA_H_

#include <stdint.h>
#include <string.h>



/************************************MODBUS 命令结构*********************************************/
/* Function codes */
#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_FILE                0x15
#define _FC_WRITE_AND_READ_REGISTERS  0x17


#define CRC16_SIZE                      sizeof(uint16_t)        
#define REG_SIZE                        sizeof(uint16_t)
#define SWAPBYTE(a)                     (((a & 0x00ff) << 8) | ((a & 0xff00) >> 8)) 
#define SWAPINT(a)                      (((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | ((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) )     

#define SWAP_8BIT(a)                      (((a & 0x000000ff) << 8) | ((a & 0x0000ff00) >> 8) | ((a & 0xff000000) >> 8) | ((a & 0x00ff0000) << 8) )     

#define SWAP_16BIT(x)                   (((x & 0x00ff) << 8) | ((x & 0xff00) >> 8))
#define SWAP_32BIT(x)                   (((x & 0xffff0000) >> 16) | ((x & 0x0000ffff) << 16))

#define MB_ALIGN_PT_16BIT_READ(p)       (*((uint8_t *)(p)) << 8 | *((uint8_t *)(p)+1)) 
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)


/*
    MODBUS 模式 
*/
enum
{
    MODBUS_RTU,      
    MODBUS_TCP,
    MODBUS_RTU_EXT
};



#pragma pack(push,1)

/* 
*   ModBus MBAP公共头
*/
typedef struct tagMBAPHead 
{
    uint16_t        TransmitFlag;
    uint16_t        PotocolFlag;
    uint16_t        Len;   
} *LPModbusMBAPHead;


/* 
*   ModBus 公共头
*/
typedef struct tagCommonHead
{
    uint8_t     SlaveAddr;
    uint8_t     Cmd;   
} *LPModBusHEAD;



/*
*   读继电器状态 01H
*/
typedef struct tagReadRly01H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 01H 读继电器
    uint16_t    wRegAddr;        // 继电器地址
    uint16_t    wRegNum;         // 继电器个数   
}  *LPModBusReadRly01H;

/*
*   读继电器状态回应 01H
*/
typedef struct tagReadRly01H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 01H 读继电器
    uint8_t     ByteNum;        // 字节数
}  *LPModBusReadRly01H_ack;


/*
*   读开关量状态 02H
*/
typedef struct tagReadDI02H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 02H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
}  *LPModBusReadDI02H;

/*
*   读开关量状态回应 02H
*/
typedef struct tagReadDI02H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 02H 读开关量
    uint8_t     ByteNum;        // 字节数       
}  *LPModBusReadDI02H_ack;



/*
*   读寄存器状态 03H
*/
typedef struct tagReadREG03H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
} *LPModBusReadREG03H;

/*
*   读寄存器回应 03H
*/
typedef struct tagReadREG03H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 03H 读寄存器
    uint8_t     nbytes;         // 字节个数  （寄存器 * 2）  
} ModBusReadREG03H_ack,*LPModBusReadREG03H_ack;



/*
*   读输入寄存器状态 04H
*/
typedef struct tagReadREG04H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 04H 读开关量
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
} *LPModBusReadREG04H;


/*
*   读寄存器回应 04H
*/
typedef struct tagReadREG04H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 04H 读寄存器
    uint8_t     nbytes;         // 字节个数  （寄存器 * 2）
} *LPModBusReadREG04H_ack;


/*
*   写继电器 05H
*/
typedef struct tagWriteRly05H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 02H 写继电器
    uint16_t    RlyAddr;        // 继电器地址
    uint16_t    RlyVal;         // 继电器控制
} *LPModBusWriteRly05H;

/*
*   写继电器回应  05H
*/
typedef struct tagWriteRly05H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 02H 写继电器
    uint16_t    RlyAddr;        // 继电器地址
    uint16_t    RlyVal;         // 继电器控制
} *LPModBusWriteRly05H_ack;



/*
*   写单保持寄存器（模拟量输出） 06H
*/
typedef struct tagWriteReg06H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 06H 写寄存器
    uint16_t    RegAddr;        // 地址
    uint16_t    RegVal;         // 
} *LPModBusWriteReg06H;

/*
*   写单保持寄存器（模拟量输出） 06H,需要写密码的
*/
typedef struct tagWriteSysReg06H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 06H 写寄存器
    uint16_t    RegAddr;        // 地址
    uint16_t    Password;       //密码
    uint16_t    RegVal;         // 
} *LPModBusWriteSysReg06H;

/*
*   写单保持寄存器（模拟量输出） 06H
*/
typedef struct tagWriteReg06H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 02H 写寄存器
    uint16_t    RegAddr;        // 
    uint16_t    RegVal;         // 
} *LPModBusWriteReg06H_ack;


/*
*   写寄存器  10H
*/
typedef struct tagWriteREG10H
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 10H 读寄存器
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
    uint8_t     BytesNum;       // 
} *LPModBusWriteREG10H;

/*
*   写寄存器回应  10H
*/
typedef struct tagWriteREG10H_ack
{
    uint8_t     SlaveAddr;      // 从机地址
    uint8_t     Cmd;            // 10H 读寄存器
    uint16_t    wRegAddr;        // 寄存器地址
    uint16_t    wRegNum;         // 寄存器个数
} *LPModBusWriteREG10H_ack;

#pragma pack(pop)

#endif
