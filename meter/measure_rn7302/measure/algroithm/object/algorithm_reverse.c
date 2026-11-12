/**
 * @file        algorithm_reverse.c
 * @brief       CT 反向时翻转有功 / 无功 / 功因符号
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-11-03  yansh    1.0.0   反向
 */
#include <stddef.h>
#include "algorithm_reverse.h"

/**
 * @brief   CT 反向：按 phase_adjust bit15 翻转对应相 P/Q/PF
 *
 * 仅 WIREMODE_P3W4 生效；非三线四线直接返回 0。
 * 翻转后重算 Ptot/Qtot；若任一相反向且 Stot≠0，按 Ptot/Stot 重算 PFtot。
 * 半波相序关闭路径在功因之后调用；相序开启路径角已含极性，不再调用。
 *
 * @param[in,out] out  读写功率与功因
 * @param[in]     cfg  wiremode、user.phase_adjust[3]
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_reverse(struct msr_data_realtime *out,
                            const struct msr_config *cfg)
{
    int phase_A = 1, phase_B = 1, phase_C = 1;

    if (out == NULL || cfg == NULL) {
        return -1;
    }

    if (cfg->user.wiremode != WIREMODE_P3W4) {
        return 0;
    }

    if ((cfg->user.phase_adjust[0] & 0x8000U) == 0x8000U) {
        phase_A = -1;
    }
    if ((cfg->user.phase_adjust[1] & 0x8000U) == 0x8000U) {
        phase_B = -1;
    }
    if ((cfg->user.phase_adjust[2] & 0x8000U) == 0x8000U) {
        phase_C = -1;
    }

    out->Pa   = out->Pa * phase_A;
    out->Pb   = out->Pb * phase_B;
    out->Pc   = out->Pc * phase_C;
    out->Ptot = out->Pa + out->Pb + out->Pc;

    out->Qa   = out->Qa * phase_A;
    out->Qb   = out->Qb * phase_B;
    out->Qc   = out->Qc * phase_C;
    out->Qtot = out->Qa + out->Qb + out->Qc;

    out->PFa = (int16_t)(out->PFa * phase_A);
    out->PFb = (int16_t)(out->PFb * phase_B);
    out->PFc = (int16_t)(out->PFc * phase_C);

    if (((phase_A + phase_B + phase_C) != 3) && (out->Stot != 0)) {
        out->PFtot = (int16_t)((float)out->Ptot / (float)out->Stot * 1000.0f);
    }

    return 0;
}
