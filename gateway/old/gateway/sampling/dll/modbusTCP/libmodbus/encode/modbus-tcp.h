#ifndef _MODBUS_TCP_H_
#define _MODBUS_TCP_H_

#include <stdint.h>
#include "modbus-data.h"
#include "modbus-rtu.h"


extern const struct modbus_encode_decode mb_tcp_encode_decode;

uint16_t modbus_TransmitFlagInc(void);

#endif // #ifndef _MODBUS_RTU_H_

