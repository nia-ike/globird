#ifndef _DLT698_45_DATA_H_
#define _DLT698_45_DATA_H_

#include <stdint.h>

#define _FC_DLT698_45_START_FLAG        0x68
#define _FC_DLT698_45_END_FLAG          0x16
#define _FC_DLT698_45_READ_DATA         0x11

#define DLT698_45_DEFAULT_PHYADDR       0ULL
#define DLT698_45_BROADCAST_PHYADDR     0xAAAAAAAAAAAAULL

#define SWAP_UINT32(a) ((((a) & 0x000000FFU) << 24) | (((a) & 0x0000FF00U) << 8) | (((a) & 0x00FF0000U) >> 8) | (((a) & 0xFF000000U) >> 24))

#pragma pack(push, 1)
/* 前导 0xFE×4 放在发送缓冲区中、本结构体之前，勿放在结构体内，否则 memset 会清成 00 */
typedef struct
{
    uint8_t start_flag;
    uint16_t length;
    uint8_t control;
    uint8_t sa;
    uint8_t addr[6];
    uint8_t client_addr;
    uint16_t hcs;
    uint8_t apdu[7]; /* GET-Request-Normal: 05 01 + PIID + OAD(4B) */
    uint16_t fcs;
    uint8_t end_flag;
} dlt698_45_request_t;
#pragma pack(pop)

#endif
