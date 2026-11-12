/**
 * @file        rn7302_phasecheck.c
 * @brief       RN7302相位检查
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302相位检查
 */
#include <stddef.h>
#include <stdio.h>
#include "rn7302_phasecheck.h"
#include "rn7302_register.h"
#include "port_rn7302.h"

/**
 * @brief   RN7302相序重映射检查是否发生
 * @param[in] cfg    配置
 * @return  PHASECHECK_NOT_HAPPENED:未发生;PHASECHECK_HAPPENED:发生
 */
uint16_t rn7302_phasecheck_is_happened(struct msr_config *cfg)
{
    /* 低3位为1/2/3，即相序未换；bit15表示CT反向不开启硬件换相 */
    if ((1U == (cfg->user.phase_adjust[0] & 0x07U))
    &&  (2U == (cfg->user.phase_adjust[1] & 0x07U))
    &&  (3U == (cfg->user.phase_adjust[2] & 0x07U))) {
        return PHASECHECK_NOT_HAPPENED;
    }
    else {
        return PHASECHECK_HAPPENED;
    }
}

/**
 * @brief   RN7302相位检查初始化
 * @param[in] cfg    配置
 * @return  void
 */
void rn7302_phasecheck_init(struct msr_config *cfg)
{
    if (cfg == NULL) {
        return;
    }

    uint32_t emucfg = 0;
    uint32_t tfeec  = 0;
    uint32_t ichsel = 0;

    switch (cfg->user.phase_adjust_mode)
    {
    /* 电压相序调整配置 */
    case PHASEAJDMODE_VOLTAGE:
        printf("电压相序调整逻辑暂未实现\n");
        break;
    /* 电流相序调整配置 */
    case PHASEADJMODE_CURRENT:
    default:
        if (PHASECHECK_NOT_HAPPENED == rn7302_phasecheck_is_happened(cfg)) {
            emucfg = 0;
            tfeec  = 0;
            ichsel = 0;
        }
        else {
            emucfg = 0;
            tfeec  = 0;
            ichsel = 0;
        }

        MeterChipWriteReg(RN7302_REG_EMUCFG, RN7302_LEN_EMUCFG, emucfg);
        MeterChipWriteReg(RN7302_REG_TFEEC_EN, RN7302_LEN_TFEEC_EN, tfeec);
        MeterChipWriteReg(RN7302_REG_ICHSEL, RN7302_LEN_ICHSEL, ichsel);
        break;
    }
}
