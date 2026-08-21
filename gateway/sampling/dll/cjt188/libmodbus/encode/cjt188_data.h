#ifndef _CJT188_DATA_H_
#define _CJT188_DATA_H_

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "crc16.h"


/************************************MODBUS 命令结构*********************************************/
/*CJT188格式关键字段的长度*/
#define CJT188_BROADCAST_ADDR_FLAG      0x99
#define CJT188_WILDCARD_FLAG            0xAA

/*数据域的长度，决定数据格式*/
#define CJT188_READDATA_LEN             3

#define CJT188_CRC_SIZE                  sizeof(uint8_t)
#define CJT188_ENDFLAG_SIZE              sizeof(uint8_t)
#define CJT188_DATAID_SIZE               2
#define CJT188_COMMONFORM_SIZE           11

#define CJT188_COMM_ADDR_SIZE			7
#define CJT188_DATE_ID_SIZE			    2
#define CJT188_W_PASSWORD_SIZE			4
#define CJT188_C_EVENT_ID_SIZE			4


/* Function codes */
#define _FC_CJT188_START_FLAG             0x68
#define _FC_CJT188_END_FLAG               0x16

#define CJT188_ACK_SIZE_16H			      0x16
#define CJT188_ACK_SIZE_2EH			      0x2E


/* Function codes */
#define _FC_CJT188_BROADCAST_TIME                      0x08

#define _FC_CJT188_READ_DATA                           0x01  /*读数据：主站请求帧，加密传输*/
#define _FC_CJT188_READ_DATA_ACK_OK              	   0x81  /*读数据：从站正常应答帧*/
#define _FC_CJT188_READ_DATA_ACK_NG		               0xC1  /*读数据：从站异常应答帧*/

#define _FC_CJT188_READ_VER                            0x09  /*读密钥版本号：主站请求帧，明码传输*/
#define _FC_CJT188_READ_VER_ACK_OK              	   0x89  /*读密钥版本号：从站正常应答帧*/
#define _FC_CJT188_READ_VER_ACK_NG		               0xC9  /*读密钥版本号：从站异常应答帧*/

#define _FC_CJT188_READ_COMMADDR                       0x03	 /*读地址：主站请求帧，单机通讯，明码传输*/
#define _FC_CJT188_READ_COMMADDR_ACK_OK                0x83	 /*读地址：从站正常应答帧*/
#define _FC_CJT188_READ_COMMADDR_ACK_NG                0xC3  /*读地址：从站异常应答帧*/

#define _FC_CJT188_READ_DATA_10H                       0x10  /*冷水水表*/
#define _FC_CJT188_READ_DATA_11H                       0x11  /*生活热水水表*/
#define _FC_CJT188_READ_DATA_12H                       0x12  /*直饮水水表*/
#define _FC_CJT188_READ_DATA_13H                       0x13  /*中水水表*/
#define _FC_CJT188_READ_DATA_20H                       0x20  /*热量表(计热量)*/
#define _FC_CJT188_READ_DATA_21H                       0x21  /*热量表(计冷量)*/
#define _FC_CJT188_READ_DATA_30H                       0x30  /*燃气表*/
#define _FC_CJT188_READ_DATA_40H                       0x40  /*其他仪表，如电度表*/



#define CRC16_SIZE                      sizeof(uint16_t)        
#define REG_SIZE                        sizeof(uint16_t)
#define SWAPBYTE(a)                     (((a & 0x00ff) << 8) | ((a & 0xff00) >> 8)) 
#define SWAPINT(a)                      (((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | ((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) )     

#define SWAP_8BIT(a)                      (((a & 0x000000ff) << 8) | ((a & 0x0000ff00) >> 8) | ((a & 0xff000000) >> 8) | ((a & 0x00ff0000) << 8) )     

#define SWAP_16BIT(x)                   (uint16_t)(((x) >> 8)  | ((x) << 8 ))
#define SWAP_32BIT(x)                   (uint32_t)(((x) >> 16) | ((x) << 16))

#define MB_ALIGN_PT_16BIT_READ(p)       (*((uint8_t *)(p)) << 8 | *((uint8_t *)(p)+1)) 
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)

//将uint32的a   将每个字节单独加0x33
#define BYTE_ADD_33HX(a)                      ((a & 0x000000ff)+0x33) | ((a & 0x0000ff00)+(0x33<<8)) | ((a & 0xff000000)+(0x33<<24)) | ((a & 0x00ff0000)+(0x33<<16))

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
	uint8_t StartFlag;						    /*帧起始符*/
	uint8_t DevType;         					/*仪表类型*/	
	uint8_t Addr[CJT188_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t CtlCode;         					/*控制码*/
} *LPCJT188HEAD;



/*
*   读数据 01H
*/
typedef struct tagReadREG01H
{
	uint8_t StartFlag;						    /*帧起始符*/
	uint8_t DevType;         					/*仪表类型*/	
	uint8_t Addr[CJT188_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[CJT188_DATE_ID_SIZE];	    /*数据域*/ 
	uint8_t SER;							    /*序列号*/
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} *LPCJT188ReadREG01H;



/*
*   81H，读数据回应 01H, 从站正常应答帧
*/
typedef struct tagReadREG01H_ack
{
	uint8_t StartFlag;						    /*帧起始符*/
	uint8_t DevType;         					/*仪表类型*/	
	uint8_t Addr[CJT188_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[CJT188_DATE_ID_SIZE];		/*数据标识*/
	uint8_t SER;							    /*序列号*/
	uint8_t *pFrameData;	    				/*数据域*/ 
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} CJT188ReadREG01H_ack,*LPCJT188ReadREG01H_ack;



#pragma pack(pop)

#endif
