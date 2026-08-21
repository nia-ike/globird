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

#ifndef _MB_PROTOCOL_H_
#define _MB_PROTOCOL_H_

#include <stdint.h>
#include "mb_cfg.h"
#include "mb_com_baseclass.h"

#define __LITTLE_ENDIAN__               1

#define _FC_READ_HOLDING_REGISTERS      0x03
#define _FC_READ_INPUT_REGISTERS        0x04
#define _FC_WRITE_COIL_REGISTERS        0x05
#define _FC_WRITE_SINGLE_REGISTERS      0x06
#define _FC_WRITE_MULTIPLE_REGISTERS    0x10
#define _FC_GET_FILE_INFO               0x44
#define _FC_SND_FILE_PKG                0x45
#define _FC_GET_FILE_HANDLE             0x46
#define _FC_SND_FILE_HANDLE             0x47
#define _FC_SHELL_COMMAND               0x48
#define _FC_DEBUG_MESSAGE               0x49
#define _FC_UNINSTALL_APP               0x4A


#define CRC16_SIZE                      sizeof(uint16_t)        
#define REG_SIZE                        sizeof(uint16_t)
#define BIT32_SIZE                      sizeof(uint32_t)

/**/
#define MB_ALIGN_PT_16BIT_READ(p)       (*((uint8_t *)(p)) << 8 | *((uint8_t *)(p)+1)) 
#define MB_ALIGN_PT_16BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 8 & 0xff; (*((uint8_t*)p+1)) = v & 0xff;}while(0)
#define MB_ALIGN_PT_32BIT_WRITE(p,v)    do {(*((uint8_t*)p)) = v >> 24 & 0xff; (*((uint8_t*)p+1)) = v >> 16 & 0xff; (*((uint8_t*)p+2)) = v >> 8 & 0xff; (*((uint8_t*)p+3)) = v  & 0xff;}while(0)

#if __LITTLE_ENDIAN__
#define SWAP_16BIT(a)                    ((((a) & 0xff) << 8) | (((a) >> 8) & 0xff)) 
#define SWAP_32BIT(a)                    (((a & 0x000000ff) << 24) | ((a & 0x0000ff00) << 8) | ((a & 0xff000000) >> 24) | ((a & 0x00ff0000) >> 8) ) 
#define SWAP_32BIT_BADC(a)               

#else
#define SWAP_16BIT(a)                   a
#define SWAP_32BIT(a)  	                a 
#endif







int mb_msg_process_rtu(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);
int mb_msg_process_tcp(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);
int mb_msg_process_Dlt645_07(struct mb_comm_ctx_t *pCtx, uint8_t *pRecvBuff, uint16_t recvLen);

#endif

