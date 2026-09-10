/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:         
  History:              
*************************************************/
#ifndef __RN7302_PHASECHECK_H__
#define __RN7302_PHASECHECK_H__
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include "db_struct.h"

#ifdef  __cplusplus
extern "C" {
#endif

enum
{
    ICHSEL_OFF = 0,
    ICHSEL_ON  = 1,
};


/**
 * @brief : 获取电流通道选择标志
 * @return: 电流通道选择
 */
uint16_t get_ICHSEL_flag(void);

/**
 * @brief : 根据 PhaseAdjust 返回物理电流通道索引
 * @param : phase_adj: PhaseAdjust 值，低 3 位 1/2/3 对应物理 A/B/C
 * @return: 通道索引 0/1/2；非法值回退为 0（A 相）
 */
uint16_t Rn7302_phase_adj_to_ch_idx(uint16_t phase_adj);

/**
 * @brief : 初始化电流通道选择
 * @param : pCfgUser: 用户配置参数指针
 * @return: None
 */
void Rn7302_ICHSEL_Init(struct db_cfg_user_para *pCfgUser);


#ifdef  __cplusplus
}
#endif

#endif
