/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#ifndef __RN7302_READ_H__
#define __RN7302_READ_H__
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "Rn7302_register.h"


#ifdef  __cplusplus
extern "C" {
#endif

/* С��ģʽCPU ���ú� */
#define __LITTLE_ENDIAN__             1 


struct rn7302_emu * get_EmuFull(void);
struct rn7302_emu * get_EmuFund(void);
struct rn7302_emu * get_EmuHalf(void);

struct rn7302_emu_energy * get_EmuFullEnergy(void);
struct rn7302_emu_energy * get_EmuFundEnergy(void);


#ifdef  __cplusplus
}
#endif

#endif

