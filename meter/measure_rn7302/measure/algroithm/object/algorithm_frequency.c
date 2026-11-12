/**
 * @file        algorithm_frequency.c
 * @brief       频率计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   频率计算
 */
#include <stddef.h>
#include "algorithm_frequency.h"

/**
 * @brief   频率合法性与无压判零
 *
 * Freq_hp 越出 [0.7, 1.3]×额定则钳到额定 50Hz；
 * P3W4 三相相电压全 0、其它接线三线电压全 0 时 Freq_hp 置 0。
 *
 * @param[in,out] out  读写 Freq_hp；读电压幅值做无压判断
 * @param[in]     cfg  接线模式
 */
static void frequency_judge(struct msr_data_realtime *out, const struct msr_config *cfg)
{
    if ((out->Freq > (uint32_t)MULTIPLE_UFREQ * MSR_UFREQ_MAX)
        || (out->Freq < (uint32_t)MULTIPLE_UFREQ * MSR_UFREQ_MIN)) {
        out->Freq = (uint32_t)MSR_UFREQ_NOM * MULTIPLE_UFREQ;
    }

    if (cfg->user.wiremode == WIREMODE_P3W4) {
        if ((out->Va == 0) && (out->Vb == 0) && (out->Vc == 0)) {
            out->Freq = 0;
        }
    } else {
        if ((out->Vab == 0) && (out->Vbc == 0) && (out->Vca == 0)) {
            out->Freq = 0;
        }
    }
}

/**
 * @brief   仅计算频率×10000（半波 V/I 滤波用，不写 out）
 *
 * @param[in] in  含 UFreq；NULL 或 UFreq==0 时返回 0
 *
 * @return Hz_hp（×10000）
 */
uint32_t algorithm_frequency_hw(const struct msr_input_fullfund *in)
{
    if (in == NULL || in->UFreq == 0) {
        return 0;
    }

    return (uint32_t)((uint64_t)MSR_FOSC * 8 * MULTIPLE_UFREQ / (uint32_t)in->UFreq + 1);
}

/**
 * @brief   频率（全波 / 基波 / 半波共用）
 *
 * Freq_hp = FOSC×8×10000/UFreq+1；越界钳到额定；无电压时置 0；
 * Freq = Freq_hp/100（×100 刻度）。
 *
 * @param[out] out  写出 Freq_hp、Freq（依赖 out 中已有电压做无压判断）
 * @param[in]  cfg  接线模式
 * @param[in]  in   含 UFreq 的电参快照
 *
 * @return 0 成功；参数非法或 UFreq==0 返回 -1（并清零频率）
 */
int32_t algorithm_frequency(struct msr_data_realtime *out,
                            const struct msr_config *cfg,
                            const struct msr_input_fullfund *in)
{
    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    if (in->UFreq == 0) {
        out->Freq = 0;
        return -1;
    }

    out->Freq = (uint32_t)((uint64_t)MSR_FOSC * 8 * MULTIPLE_UFREQ
                              / (uint32_t)in->UFreq + 1);
    frequency_judge(out, cfg);
    return 0;
}
