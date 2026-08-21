#ifndef _DLT645_07_H_
#define _DLT645_07_H_

#include <stdint.h>
#include "dlt645_07_data.h"


struct dlt645_07_encode_decode
{
	int32_t (*Packet08H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum);
	int32_t (*Packet11H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address, uint32_t DataId);
	int32_t (*Packet12H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address);
	int32_t (*Packet13H)(uint8_t *pBuff, uint8_t LeadByteNum, uint64_t Address);
	int32_t (*Packet14H)(uint8_t *pBuff, uint16_t LeadByteNum, uint64_t Address, uint32_t DataId, uint32_t password, uint32_t operator,  uint16_t *pVal, uint16_t wRegNum);
	int32_t (*Packet15H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wOpenClose);
	int32_t (*Packet16H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wOpenClose,uint8_t Attr,uint16_t Password);
	int32_t (*Packet17H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
	int32_t (*Packet18H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
	int32_t (*Packet19H)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
	int32_t (*Packet1AH)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
	int32_t (*Packet1BH)(uint8_t *pBuff, uint8_t Address, uint16_t wRegAddr, uint16_t wRegNum, uint16_t *pVal,uint8_t Attr,uint16_t Password);
	int32_t (*Packet1CH)(uint8_t *pBuff, uint16_t LeadByteNum, uint64_t Address, uint32_t DataId, uint32_t password, uint32_t operator,  uint16_t *pVal, uint16_t wRegNum);
            
	int32_t (*UnPacket08H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket91H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacketD1H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacket12H)(uint8_t *pBuff, uint16_t *pVal);
	int32_t (*UnPacket93H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacket94H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacketD4H)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacket9CH)(uint8_t *pBuff, uint8_t *pVal);
	int32_t (*UnPacket15H)(uint8_t *pBuff);
	int32_t (*UnPacket16H)(uint8_t *pBuff);
	int32_t (*UnPacket17H)(uint8_t *pBuff);
	int32_t (*UnPacket18H)(uint8_t *pBuff);
	int32_t (*UnPacket19H)(uint8_t *pBuff);
	int32_t (*UnPacket1AH)(uint8_t *pBuff);
	int32_t (*UnPacket1BH)(uint8_t *pBuff);
	int32_t (*UnPacket1CH)(uint8_t *pBuff);


};


extern const struct dlt645_07_encode_decode mb_encode_decode;



#endif // #ifndef _MODBUS_RTU_H_

