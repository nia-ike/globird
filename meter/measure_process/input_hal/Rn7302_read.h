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


/**
 * @brief  读取全波/基波/半波计量寄存器
 * @param  ppEmu: 成功时返回数据指针；失败为 NULL
 * @return 0: 成功; -1: 失败（驱动 SPI/CRC 等通信错误，errno 透传）
 * @note   失败时不对外发布本拍缓存；调用方不得继续读电能寄存器
 */
int get_EmuFull(struct rn7302_emu **ppEmu);
int get_EmuFund(struct rn7302_emu **ppEmu);
int get_EmuHalf(struct rn7302_emu **ppEmu);

struct rn7302_emu_energy * get_EmuFullEnergy(void);
struct rn7302_emu_energy * get_EmuFundEnergy(void);


#ifdef  __cplusplus
}
#endif

#endif

