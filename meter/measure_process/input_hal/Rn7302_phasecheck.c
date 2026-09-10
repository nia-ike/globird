/*************************************************
  Copyright (C), 2003-2017, pilot Co., Ltd.
  File name:      Rn7302.h
  Author:       longzh    Version: 1.0       Date: 2017-6-9
  Description:  Rn7302 ����
  Others:
  History:
*************************************************/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include "math.h"
#include "Rn7302.h"
#include "Rn7302_read.h"
#include "Rn7302_adjust.h"
#include "Rn7302File.h"
#include <sys/time.h>
#include "debug.h"
#include "msr_process.h"
#include "Rn7302_phasecheck.h"


static uint16_t ICHSEL_flag = 0;


/**
 * @brief : 设置电流通道选择标志
 * @param : set_value: 设置值
 * @return: 电流通道选择标志
 */
static void set_ICHSEL_flag(uint16_t set_value)
{
    if ((ICHSEL_OFF != set_value) && (ICHSEL_ON != set_value))
    {
        set_value = ICHSEL_OFF;
    }

    ICHSEL_flag = set_value;
}

/**
 * @brief : 获取电流通道选择标志
 * @return: 电流通道选择
 */
uint16_t get_ICHSEL_flag(void)
{
    uint16_t flag = ICHSEL_flag;

    if((ICHSEL_OFF != flag) && (ICHSEL_ON != flag))
    {
        flag = ICHSEL_OFF;
    }

    return flag;
}

/**
 * @brief : 初始化电流通道选择
 * @param : pCfgUser: 用户配置参数指针
 * @return: None
 */
void Rn7302_ICHSEL_Init(struct db_cfg_user_para *pCfgUser)
{
    int32_t  tmp;
    uint16_t phase[3] = {0,0,0};

    phase[0] = pCfgUser->base.PhaseAdjust1.value;
    phase[1] = pCfgUser->base.PhaseAdjust2.value;
    phase[2] = pCfgUser->base.PhaseAdjust3.value;

    if((1 == phase[0]) && (2 == phase[1]) && (3 == phase[2]))//1、2、3分别对应ICHSEL电流切换的A/B/C的bit位
    {
        set_ICHSEL_flag(ICHSEL_OFF);

        //配置为有功无功为代数和,关闭通道切换配置位
        tmp = RN7302_REG_EMUCFG_DEFAULT ;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //关闭电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_DEFAULT;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);

        tmp = RN7302_REG_ICHSEL_DEFAULT;
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, tmp);
    }
    else
    {
        set_ICHSEL_flag(ICHSEL_ON);

        //配置为有功无功为代数和,打开通道切换配置位
        tmp = RN7302_REG_EMUCFG_TFEEC;
        MeterChipWriteReg(RN7302_REG_EMMCFG, RN7302_LEN_EMMCFG, tmp);

        //打开电流通道切换写使能
        tmp = RN7302_REG_TFEEC_EN_ENABLE;
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tmp);
        MeterChipReadReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, &tmp);

        tmp = ((phase[0] & 0x3F)) | ((phase[1] & 0x3F) << 2) | ((phase[2] & 0x3F) << 4);
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, tmp);

        MeterChipReadReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, &tmp);
    }
}
