#ifndef _IEC101_UTILS_H_
#define _IEC101_UTILS_H_

#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "cJSON.h"
#include "cJSON_Utils.h"

#define LOBYTE(w) ((uint8_t)(w))
#define HIBYTE(w) ((uint8_t)(((uint16_t)(w) >> 8) & 0xFF))

#define LOWORD(l) ((uint16_t)(l))
#define HIWORD(l) ((uint16_t)(((uint32_t)(l) >> 16) & 0xFFFF))

#define MAKEWORD(low, high) ((uint16_t)(((uint8_t)(low)) | ((uint16_t)((uint8_t)(high))) << 8))
#define MAKELONG(low, high) ((int32_t)(((uint16_t)(low)) | ((uint16_t)((uint16_t)(high))) << 16))

uint8_t CheckSum8(uint8_t *pBuff, int nLen);// 计算8位校验和

int json_get_int(const cJSON * const pObject, const char * const key, int defaultVal, bool *succeed);
float json_get_float(const cJSON * const pObject, const char * const key, float defaultVal, bool *succeed);
double json_get_double(const cJSON * const pObject, const char * const key, double defaultVal, bool *succeed);
bool json_get_bool(const cJSON * const pObject, const char * const key, bool defaultVal, bool *succeed);

#endif
