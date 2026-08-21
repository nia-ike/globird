#ifndef _DLT645_97_DATA_H_
#define _DLT645_97_DATA_H_

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "crc16.h"


/************************************MODBUS 命令结构*********************************************/
/*DLT645格式关键字段的长度*/
#define DLT645_97_DATA_FIELD_FACTOR		   0x33
#define DLT645_97_BROADCAST_ADDR_FLAG      0x99
#define DLT645_97_WILDCARD_FLAG            0xAA

/*数据域的长度，决定数据格式*/
#define DLT645_97_READDATA_M0              0
#define DLT645_97_READDATA_M1              1
#define DLT645_97_READDATA_M6              6

#define DLT645_97_CRC_SIZE                  sizeof(uint8_t)
#define DLT645_97_ENDFLAG_SIZE              sizeof(uint8_t)
#define DLT645_97_DATAID_SIZE               4
#define DLT645_97_COMMONFORM_SIZE           10

#define DLT645_97_COMM_ADDR_SIZE			6
#define DLT645_97_DATE_ID_SIZE			    2
#define DLT645_97_W_PASSWORD_SIZE			4
#define DLT645_97_C_EVENT_ID_SIZE			4


/* Function codes */
#define _FC_DLT645_97_START_FLAG             0x68
#define _FC_DLT645_97_END_FLAG               0x16


/* Function codes */
#define _FC_DLT645_97_READDATA                      0x01
#define _FC_DLT645_97_READDATA_ACK_FINISHED         0x81
#define _FC_DLT645_97_READDATA_ACK_UNFINISHED       0xA1
#define _FC_DLT645_97_READDATA_ACK_ERR              0xC1

#define _FC_DLT645_97_WRITEDATA                     0x04
#define _FC_DLT645_97_WRITEDATA_ACK_OK              0x84
#define _FC_DLT645_97_WRITEDATA_ACK_ERR             0xC4



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




#pragma pack(push,1)



/* 
*    公共头
*/
typedef struct tagCommonHead
{
	uint8_t 	StartFlag_1;						/*帧起始符*/
    uint8_t     SlaveAddr[DLT645_97_COMM_ADDR_SIZE];/*地址域*/
	uint8_t 	StartFlag_2;						/*帧起始符*/
	uint8_t 	CtlCode;         					/*控制码*/
} *LPDLT645HEAD;

typedef struct  tagDLT645_1997_FrameHead
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_97_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_97_DATE_ID_SIZE];		/*数据标识*/
}DLT645_FRAME_HEAD;


typedef struct  tagDLT645_1997_FrameDateId
{
	uint8_t DateId[DLT645_97_DATE_ID_SIZE];		/*数据标识*/
}DLT645_FRAME_DATE_ID;

typedef struct  tagDLT645_1997_FrameEnd
{
	uint8_t CS;					/*校验码*/
	uint8_t EndSymbol;			/*结束符*/
}DLT645_FRAME_END;

typedef struct  tagDLT645_1997_FrameTime
{
  uint8_t tm_min;			/* Minutes.	[0-59] */
  uint8_t tm_hour;			/* Hours.	[0-23] */
  uint8_t tm_mday;			/* Day.		[1-31] */
  uint8_t tm_mon;			/* Month.	[0-11] */
  uint8_t tm_year;			/* Year	- 1900.  */
}DLT645_FRAME_TIME;


/*
*   读数据 11H，帧格式1(m=0)
*/
typedef struct tagReadREG01H
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_97_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_97_DATE_ID_SIZE];		/*数据标识*/
	uint8_t CS;					/*校验码*/
	uint8_t EndSymbol;			/*结束符*/
} *LPDLT645_97ReadREG01H;

/*
*   读数据 11H，帧格式3(m=6，读给定时间、块数的负荷记录)
*/
typedef struct tagReadREG01H_M6
{
	DLT645_FRAME_HEAD   *pFrameHead;
	uint8_t         	*pFrameData;	/*数据域*/ 
	uint8_t         	num;	        /*负荷记录块数*/ 
	DLT645_FRAME_TIME   *pFrameTime;    /*分，时，日，月，年*/
	DLT645_FRAME_END  	*pFrameEnd;		
} *LPModBusReadREG01H_M6;


/*
*   读数据回应 11H, 从站正常应答帧，91H无后续数据帧，B1有后续数据帧
*/
typedef struct tagReadREG81H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_97_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_97_DATE_ID_SIZE];		/*数据标识*/
	uint8_t *pFrameData;	    				/*数据域*/ 
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_97ReadREG81H_ack,*LPDLT645_97ReadREG81H_ack;


/*
*   写数据 14H，主站向从站请求设置数据
*/
typedef struct tagWriteREG04H
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_97_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_97_DATE_ID_SIZE];		/*数据标识*/
	uint8_t Password[DLT645_97_W_PASSWORD_SIZE];/*写入的密码*/
	uint8_t Event[DLT645_97_C_EVENT_ID_SIZE];   /*操作者代码*/
	uint8_t pFrameData;	    				    /*数据域*/ 
//	uint8_t CS;									/*校验码*/
//	uint8_t EndSymbol;							/*结束符*/
} *LPDLT645_97WriteREG04H;


/*
*   写数据回应 14H, 从站正常应答帧，94H
*/
typedef struct tagWriteREG84H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_97_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_97WriteREG84H_ack,*LPDLT645_97WriteREG84H_ack;


#pragma pack(pop)

#endif
