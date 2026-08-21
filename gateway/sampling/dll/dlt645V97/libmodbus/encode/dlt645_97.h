#ifndef _DLT645_97_H_
#define _DLT645_97_H_

#include <stdint.h>
#include "dlt645_97_data.h"


struct dlt645_97_encode_decode
{
	int32_t (*Packet01H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t DataId);
	int32_t (*Packet04H)(uint8_t *pBuff, uint16_t LeadByteNum, uint64_t Address, uint32_t DataId, uint32_t password, uint32_t operator,  uint16_t *pVal, uint16_t wRegNum);
	
	int32_t (*UnPacket81H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacket84H)(uint8_t *pBuff, uint8_t *pVal);
	
};


extern const struct dlt645_97_encode_decode mb_encode_decode;



#endif // #ifndef _MODBUS_RTU_H_

