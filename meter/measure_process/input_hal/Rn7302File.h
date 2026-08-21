/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302File.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302?????д
  Others:         
  History:              
*************************************************/
#ifndef __RN7302FILE_H__
#define __RN7302FILE_H__
#include <stdint.h>
#include <time.h>

#ifdef  __cplusplus
extern "C" {
#endif


int MeterChipInit(int chip);
void MeterChipDestory(void);
int MeterChipReadReg(uint32_t reg, uint32_t nbyte, int32_t *pval);
int MeterChipWriteReg(uint32_t reg, uint32_t nbyte, uint32_t val);
int32_t MeterChipReadArea(uint32_t reg, uint32_t nbyte, uint32_t *pval);
const char* MeterChipGetDevicePath(void);
int MeterChipGetFd(void);


//int32_t MeterChipReadReg(RN7302_REG_(const char* file_name, RN7302_LEN_(const char* file_name,int * dest);
//int32_t Rn7302WriteFile(const char* file_name, int wrdata);
//int32_t Rn7302ReadArea(const char* file_name, void *pbuf, uint32_t len);

#ifdef  __cplusplus
}
#endif

#endif