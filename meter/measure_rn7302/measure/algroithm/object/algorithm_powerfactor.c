/**
 * @file        algorithm_powerfactor.c
 * @brief       功率因数
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   功率因数
 */
#include <stddef.h>
#include "algorithm_powerfactor.h"

/**
 * @brief   单相/合相功因：PF = P×1000/S，|P|>S 时钳到 ±1000
 *
 * @param[in] p  有功
 * @param[in] s  视在（须 >0；调用方已判零）
 * @return 功因×1000
 */
static int16_t pf_judge(int64_t p, int64_t s)
{
    int16_t pf = 0;

    if (s > 0) {
        if (p > s) {
            pf = (int16_t)POWERFACTOR_MULTIPLE;
        } else if (-p > s) {
            pf = (int16_t)(-POWERFACTOR_MULTIPLE);
        } else {
            pf = (int16_t)(p * POWERFACTOR_MULTIPLE / s);
        }
    }
    return pf;
}

/**
 * @brief   功率因数（全波 / 基波 / 半波共用）
 *
 * 使用 out 中已有 P、S：PF = P×1000/S；|P|>S 时钳到 ±1000；S==0 时 PF=0。
 * 调度须保证调用时 P、S 已按本拍策略写好（本拍视在或上一拍视在）。
 *
 * @param[in,out] out  读 Pa/Sa…，写出 PFa/PFb/PFc/PFtot
 *
 * @return 0 成功；out==NULL 返回 -1
 */
int32_t algorithm_powerfactor(struct msr_data_realtime *out)
{
    if (out == NULL) {
        return -1;
    }

    out->PFa = (out->Sa != 0) ? pf_judge(out->Pa, out->Sa) : 0;
    out->PFb = (out->Sb != 0) ? pf_judge(out->Pb, out->Sb) : 0;
    out->PFc = (out->Sc != 0) ? pf_judge(out->Pc, out->Sc) : 0;
    out->PFtot = (out->Stot != 0) ? pf_judge(out->Ptot, out->Stot) : 0;
    return 0;
}
