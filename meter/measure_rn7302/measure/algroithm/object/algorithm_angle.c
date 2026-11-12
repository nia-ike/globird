/**
 * @file        algorithm_angle.c
 * @brief       电压 / 电流相角
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   相角计算
 */
#include <stddef.h>
#include "algorithm_angle.h"
#include "rn7302_phasecheck.h"

#define ANGLE_HALF_TURN  (180 * ANGLE_MULTIPLE)

/**
 * @brief   芯片相位寄存器换算为相角
 *
 * @param[in] emu_y  YUA/YIA 等相位寄存器原始值
 *
 * @return 相角，单位 0.01°，范围 [0, 36000)
 */
static int32_t angle_emu_calc(int32_t emu_y)
{
    uint32_t val = (uint32_t)ANGLE_CONVERT(emu_y);

    return (int32_t)((ANGLE_J360_MAX - val) % ANGLE_J360_MAX);
}

/**
 * @brief   按幅值决定是否保留相角
 *
 * @param[in] value  电压/电流幅值；为 0 时相角无效
 * @param[in] angle  已换算的相角（0.01°）
 *
 * @return value!=0 时返回 angle，否则返回 0
 */
static int32_t angle_calc(uint32_t value, int32_t angle)
{
    return value ? angle : 0;
}

/**
 * @brief   相角归一化到一周内
 *
 * @param[in] angle  任意有符号相角（0.01°）
 *
 * @return 归一化结果，范围 [0, 36000)
 */
static int32_t current_angle_norm_360(int32_t angle)
{
    while (angle < 0) {
        angle += ANGLE_J360_MAX;
    }
    while (angle >= ANGLE_J360_MAX) {
        angle -= ANGLE_J360_MAX;
    }
    return angle;
}

/**
 * @brief   按 PhaseAdjust 选取物理通道电流角
 *
 * 低 3 位选 A/B/C 物理通道；bit15 置位表示 CT 反向，相角再加 180°。
 *
 * @param[in] yi_phys    物理 A/B/C 相角（已换算，0.01°）
 * @param[in] phase_adj  PhaseAdjust 配置值
 * @param[in] i_mag      逻辑相电流幅值；为 0 则返回 0
 *
 * @return 逻辑相电流相角（0.01°）
 */
static int32_t current_angle_by_phase_adjust(const int32_t yi_phys[3],
                                            uint16_t phase_adj,
                                            uint32_t i_mag)
{
    int32_t angle = yi_phys[rn7302_phasecheck_trans2idx(phase_adj)];

    if ((phase_adj & 0x8000U) != 0U) {
        angle = current_angle_norm_360(angle + ANGLE_HALF_TURN);
    }
    return angle_calc(i_mag, angle);
}

/**
 * @brief   电压幅值为 0 时清对应相角
 *
 * P3W4 看相电压 Va/Vb/Vc；P3W3 看线电压 Vab/Vbc/Vca。
 *
 * @param[in,out] out  读写 AngleU*，并读对应电压幅值
 * @param[in]     cfg  接线模式
 */
static void voltageangle_judge(struct msr_data_realtime *out,
                               const struct msr_config *cfg)
{
    if (cfg->user.wiremode == WIREMODE_P3W4) {
        if (out->Va == 0) {
            out->AngleUa = 0;
        }
        if (out->Vb == 0) {
            out->AngleUb = 0;
        }
        if (out->Vc == 0) {
            out->AngleUc = 0;
        }
    } else if (cfg->user.wiremode == WIREMODE_P3W3) {
        if (out->Vab == 0) {
            out->AngleUa = 0;
        }
        if (out->Vbc == 0) {
            out->AngleUb = 0;
        }
        if (out->Vca == 0) {
            out->AngleUc = 0;
        }
    }
}

/**
 * @brief   电压相角（读 YUA/YUB/YUC，单位 0.01°）
 *
 * - P1W2：写 AngleUa，幅值 Va==0 时清零
 * - P3W3：清零 AngleU*（角型相角由线电压算子写出）
 * - P3W4：写三相角；对应相电压为 0 时该相角清零（可用上一拍 V）
 *
 * @param[out] out  写出 AngleUa/Ub/Uc
 * @param[in]  cfg  接线模式
 * @param[in]  in   含 YUA/YUB/YUC 的电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_voltageangle(struct msr_data_realtime *out,
                         const struct msr_config *cfg,
                         const struct msr_input_fullfund *in)
{
    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->AngleUa = (int16_t)angle_emu_calc(in->YUA);
        out->AngleUb = 0;
        out->AngleUc = 0;
        break;
    case WIREMODE_P3W3:
        /* 角型：相角由线电压算子写出 */
        out->AngleUa = 0;
        out->AngleUb = 0;
        out->AngleUc = 0;
        break;
    case WIREMODE_P3W4:
        out->AngleUa = (int16_t)angle_emu_calc(in->YUA);
        out->AngleUb = (int16_t)angle_emu_calc(in->YUB);
        out->AngleUc = (int16_t)angle_emu_calc(in->YUC);
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   电流相角（读 YIA/YIB/YIC，单位 0.01°）
 *
 * 零值判断用 out->Ia/Ib/Ic（电流算子之前调用时为上一拍幅值）。
 * phase_adjust 非默认 1/2/3 或含 bit15 时，按物理通道重映射；
 * bit15 表示 CT 反向，相角再加 180°。
 *
 * @param[out] out  写出 AngleIa/Ib/Ic；读 Ia/Ib/Ic 作幅值门控
 * @param[in]  cfg  接线模式、phase_adjust[3]
 * @param[in]  in   含 YIA/YIB/YIC 的电参快照
 *
 * @return 0 成功；参数非法或接线非法返回 -1
 */
int32_t algorithm_currentangle(struct msr_data_realtime *out,
                         const struct msr_config *cfg,
                         const struct msr_input_fullfund *in)
{
    int32_t  yi_phys[3];
    uint16_t adj1, adj2, adj3;
    int      phase_adjusted;

    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    adj1 = cfg->user.phase_adjust[0];
    adj2 = cfg->user.phase_adjust[1];
    adj3 = cfg->user.phase_adjust[2];

    yi_phys[0] = angle_emu_calc(in->YIA);
    yi_phys[1] = angle_emu_calc(in->YIB);
    yi_phys[2] = angle_emu_calc(in->YIC);

    phase_adjusted = ((adj1 != 1U) || (adj2 != 2U) || (adj3 != 3U) ||
                      ((adj1 | adj2 | adj3) & 0x8000U) != 0U);

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        if (phase_adjusted) {
            out->AngleIa = (int16_t)current_angle_by_phase_adjust(
                yi_phys, adj1, out->Ia);
        } else {
            out->AngleIa = (int16_t)angle_calc(out->Ia, yi_phys[0]);
        }
        out->AngleIb = 0;
        out->AngleIc = 0;
        break;
    case WIREMODE_P3W3:
    case WIREMODE_P3W4:
        if (phase_adjusted) {
            out->AngleIa = (int16_t)current_angle_by_phase_adjust(
                yi_phys, adj1, out->Ia);
            out->AngleIb = (int16_t)current_angle_by_phase_adjust(
                yi_phys, adj2, out->Ib);
            out->AngleIc = (int16_t)current_angle_by_phase_adjust(
                yi_phys, adj3, out->Ic);
        } else {
            out->AngleIa = (int16_t)angle_calc(out->Ia, yi_phys[0]);
            out->AngleIb = (int16_t)angle_calc(out->Ib, yi_phys[1]);
            out->AngleIc = (int16_t)angle_calc(out->Ic, yi_phys[2]);
        }
        break;
    default:
        return -1;
    }

    return 0;
}
