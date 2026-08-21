#ifndef _DLT645_07_DATA_H_
#define _DLT645_07_DATA_H_

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "crc16.h"


/************************************MODBUS 命令结构*********************************************/
/*DLT645格式关键字段的长度*/
#define DLT645_07_DATA_FIELD_FACTOR		   0x33
#define DLT645_07_BROADCAST_ADDR_FLAG      0x99
#define DLT645_07_WILDCARD_FLAG            0xAA

/*数据域的长度，决定数据格式*/
#define DLT645_07_READDATA_M0              0
#define DLT645_07_READDATA_M1              1
#define DLT645_07_READDATA_M6              6

#define DLT645_07_CRC_SIZE                  sizeof(uint8_t)
#define DLT645_07_ENDFLAG_SIZE              sizeof(uint8_t)
#define DLT645_07_DATAID_SIZE               4
#define DLT645_07_COMMONFORM_SIZE           10

#define DLT645_07_COMM_ADDR_SIZE			6
#define DLT645_07_DATE_ID_SIZE			    4
#define DLT645_07_W_PASSWORD_SIZE			4
#define DLT645_07_C_EVENT_ID_SIZE			4


/* Function codes */
#define _FC_DLT645_07_START_FLAG             0x68
#define _FC_DLT645_07_END_FLAG               0x16


/* Function codes */
#define _FC_DLT645_07_BROADCAST_TIME                      0x08

#define _FC_DLT645_07_READ_DATA                           0x11  /*读数据：主站请求帧，请求读电能表数据*/
#define _FC_DLT645_07_READ_DATA_ACK_FINISHED              0x91  /*读数据：从站正常应答帧，无后续数据帧*/
#define _FC_DLT645_07_READ_DATA_ACK_UNFINISHED            0xB1  /*读数据：从站正常应答帧，有后续数据帧*/
#define _FC_DLT645_07_READ_DATA_ACK_ERR                   0xD1  /*读数据：从站异常应答帧*/

#define _FC_DLT645_07_READ_FOLLOW_DATA                    0x12  /*读后续数据：主站请求帧，请求读后续数据*/
#define _FC_DLT645_07_READ_FOLLOW_DATA_ACK_FINISHED       0x92  /*读后续数据：从站正常应答帧，无后续数据帧*/
#define _FC_DLT645_07_READ_FOLLOW_DATA_ACK_UNFINISHED     0xB2  /*读后续数据：从站正常应答帧，有后续数据帧*/
#define _FC_DLT645_07_READ_FOLLOW_DATA_ACK_ERR            0xD2  /*读后续数据：从站异常应答帧*/

#define _FC_DLT645_07_READ_COMMADDR                       0x13
#define _FC_DLT645_07_READ_COMMADDR_ACK_OK                0x93

#define _FC_DLT645_07_WRITE_DATA                          0x14  /*写数据：主站请求帧，主站向从站请求设置数据(或编程)*/
#define _FC_DLT645_07_WRITE_DATA_ACK_OK                   0x94  /*写数据：从站异常应答帧*/
#define _FC_DLT645_07_WRITE_DATA_ACK_ERR                  0xD4

#define _FC_DLT645_07_WRITE_COMMADDR                      0x15
#define _FC_DLT645_07_WRITE_COMMADDR_ACK_OK               0x95

#define _FC_DLT645_07_READ_FREEZE                         0x16
#define _FC_DLT645_07_READ_FREEZE_ACK_OK                  0x96
#define _FC_DLT645_07_READ_FREEZE_ACK_ERR                 0xD6

#define _FC_DLT645_07_WRITE_BAURATE                       0x17
#define _FC_DLT645_07_WRITE_BAURATE_ACK_OK                0x97
#define _FC_DLT645_07_WRITE_BAURATE_ACK_ERR               0xD7

#define _FC_DLT645_07_WRITE_PASSWORD                      0x18
#define _FC_DLT645_07_WRITE_PASSWORD_ACK_OK               0x98
#define _FC_DLT645_07_WRITE_PASSWORD_ACK_ERR              0xD8

#define _FC_DLT645_07_WRITE_CLEAR_DEMAND                  0x19
#define _FC_DLT645_07_WRITE_CLEAR_DEMAND_ACK_OK           0x99
#define _FC_DLT645_07_WRITE_CLEAR_DEMAND_ACK_ERR          0xD9

#define _FC_DLT645_07_WRITE_CLEAR_ALL                     0x1A
#define _FC_DLT645_07_WRITE_CLEAR_ALL_ACK_OK              0x9A
#define _FC_DLT645_07_WRITE_CLEAR_ALL_ACK_ERR             0xDA

#define _FC_DLT645_07_WRITE_CLEAR_SOE                     0x1B
#define _FC_DLT645_07_WRITE_CLEAR_SOE_ACK_OK              0x9B
#define _FC_DLT645_07_WRITE_CLEAR_SOE_ACK_ERR             0xDB

#define _FC_DLT645_07_WRITE_CONTROL                       0x1C
#define _FC_DLT645_07_WRITE_CONTROL_ACK_OK                0x9C
#define _FC_DLT645_07_WRITE_CONTROL_ACK_ERR               0xDC



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
	uint8_t 	StartFlag_1;						/*帧起始符*/
    uint8_t     SlaveAddr[DLT645_07_COMM_ADDR_SIZE];/*地址域*/
	uint8_t 	StartFlag_2;						/*帧起始符*/
	uint8_t 	CtlCode;         					/*控制码*/
} *LPDLT645HEAD;

typedef struct  tagDLT645_2007_FrameHead
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_07_DATE_ID_SIZE];		/*数据标识*/
}DLT645_FRAME_HEAD;


typedef struct  tagDLT645_2007_FrameDateId
{
	uint8_t DateId[DLT645_07_DATE_ID_SIZE];		/*数据标识*/
}DLT645_FRAME_DATE_ID;

typedef struct  tagDLT645_2007_FrameEnd
{
	uint8_t CS;					/*校验码*/
	uint8_t EndSymbol;			/*结束符*/
}DLT645_FRAME_END;

typedef struct  tagDLT645_2007_FrameTime
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
typedef struct tagReadREG11H
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_07_DATE_ID_SIZE];		/*数据标识*/
	uint8_t CS;					/*校验码*/
	uint8_t EndSymbol;			/*结束符*/
} *LPDLT645_07ReadREG11H;

/*
*   读数据 11H，帧格式3(m=6，读给定时间、块数的负荷记录)
*/
typedef struct tagReadREG11H_M6
{
	DLT645_FRAME_HEAD   *pFrameHead;
	uint8_t         	*pFrameData;	/*数据域*/ 
	uint8_t         	num;	        /*负荷记录块数*/ 
	DLT645_FRAME_TIME   *pFrameTime;    /*分，时，日，月，年*/
	DLT645_FRAME_END  	*pFrameEnd;		
} *LPModBusReadREG11H_M6;


/*
*   读数据回应 11H, 从站正常应答帧，91H无后续数据帧，B1有后续数据帧
*/
typedef struct tagReadREG91H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_07_DATE_ID_SIZE];		/*数据标识*/
	uint8_t *pFrameData;	    				/*数据域*/ 
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_07ReadREG91H_ack,*LPDLT645_07ReadREG91H_ack;

/*
*   读数据回应 11H, 从站异常应答帧，D1H
*/
typedef struct tagReadREGD1H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t *pFrameData;	    				/*数据域*/ 
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_07ReadREGD1H_ack,*LPDLT645_07ReadREGD1H_ack;

/*
*   读数据 13H，读通讯地址
*/
typedef struct tagReadREG13H
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} *LPDLT645_07ReadREG13H;


/*
*   读数据回应 13H, 从站正常应答帧，93H
*/
typedef struct tagReadREG93H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t *pFrameData;	    				/*数据域*/ 
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_07ReadREG93H_ack,*LPDLT645_07ReadREG93H_ack;


/*
*   写数据 14H，主站向从站请求设置数据
*/
typedef struct tagWriteREG14H
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t DataId[DLT645_07_DATE_ID_SIZE];		/*数据标识*/
	uint8_t Password[DLT645_07_W_PASSWORD_SIZE];/*写入的密码*/
	uint8_t Event[DLT645_07_C_EVENT_ID_SIZE];   /*操作者代码*/
	uint8_t pFrameData;	    				    /*数据域*/ 
//	uint8_t CS;									/*校验码*/
//	uint8_t EndSymbol;							/*结束符*/
} *LPDLT645_07WriteREG14H;


/*
*   写数据回应 14H, 从站正常应答帧，94H
*/
typedef struct tagWriteREG94H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_07WriteREG94H_ack,*LPDLT645_07WriteREG94H_ack;


/*
*   写数据回应 14H, 从站异常应答帧，D4H
*/
typedef struct tagWriteREGD4H_ack
{
	uint8_t StartFlag_1;						/*帧起始符*/
	uint8_t Addr[DLT645_07_COMM_ADDR_SIZE];	    /*地址域*/
	uint8_t StartFlag_2;						/*帧起始符*/
	uint8_t CtlCode;         					/*控制码*/
    uint8_t DataLen;         					/*数据域长度*/
	uint8_t CS;									/*校验码*/
	uint8_t EndSymbol;							/*结束符*/
} DLT645_07WriteREGD4H_ack,*LPDLT645_07WriteREGD4H_ack;

#pragma pack(pop)

#endif
