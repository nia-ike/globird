#ifndef _CJT188_H_
#define _CJT188_H_

#include <stdint.h>
#include "cjt188_data.h"


struct cjt188_encode_decode
{
	int32_t (*Packet01H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t DataId, uint8_t DevType);    
	int32_t (*UnPacket01H)(uint8_t *pBuff, uint8_t *pVal);
};


extern const struct cjt188_encode_decode mb_encode_decode;



#endif // #ifndef _MODBUS_RTU_H_

