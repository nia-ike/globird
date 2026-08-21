#ifndef _DLT698_45_H_
#define _DLT698_45_H_

#include <stdint.h>

struct dlt698_45_encode_decode
{
    int32_t (*Packet11H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t OAD);
    int32_t (*UnPacket91H)(uint8_t *pBuff, uint16_t len, uint32_t *pOad, double *pVal);
};

extern const struct dlt698_45_encode_decode dlt698_45_codec;

#endif
