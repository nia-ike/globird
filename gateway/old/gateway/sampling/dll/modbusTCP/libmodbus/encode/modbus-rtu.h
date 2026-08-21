#ifndef _MODBUS_RTU_H_
#define _MODBUS_RTU_H_

#include <stdint.h>
#include "modbus-data.h"


struct modbus_encode_decode
{
	int32_t (*Packet01H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum);
	int32_t (*Packet02H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum);
	int32_t (*Packet03H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum);
	int32_t (*Packet04H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum);
	int32_t (*Packet05H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wOpenClose);
	int32_t (*Packet06H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wOpenClose,uint8_t Attr,uint16_t Password);
	int32_t (*Packet10H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
            
	int32_t (*UnPacket01H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket02H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket03H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket04H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket05H)(uint8_t *pBuff);
	int32_t (*UnPacket06H)(uint8_t *pBuff);
	int32_t (*UnPacket10H)(uint8_t *pBuff);


};


extern const struct modbus_encode_decode mb_encode_decode;



#endif // #ifndef _MODBUS_RTU_H_

