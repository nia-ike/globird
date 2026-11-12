/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：algorithm_phase_adjust_reverse.c
*  描    述：PhaseAdjust bit15 极性反向——软件翻功率符号（电流/电压模式共用）
*
*  当前版本：V1.1
*  修改记录
*  作者       时间         版本      修改描述
*  raoy      2021       V1.00      创建（原 CurrentReverse，仅电流）
*           2026       V1.1       电压/电流模式统一；更名 PhaseAdjustReverse
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include "Rn7302.h"
#include "Rn7302File.h"
#include "Rn7302_phasecheck.h"
#include "algorithm_phase_adjust_reverse.h"

/*********************************************************************************************************
** Function name:       rms_algorithm_PhaseAdjustReverse
** Descriptions:        按 PhaseAdjust bit15 翻各相 P/Q/PF（P3W4；电流/电压模式均适用）
*********************************************************************************************************/
int32_t rms_algorithm_PhaseAdjustReverse(struct db_database_measure_base_type *pMeasureBase,
                                         struct db_basic_realtime_type *out,
                                         uint32_t pt,
                                         uint32_t ct)
{
    int  phase_A = 1, phase_B = 1, phase_C = 1;
    struct db_cfg_user_para *pCfgUser = &(pMeasureBase->db_cfg.user);
    uint16_t mode = (pCfgUser->base.WiringMode.value & 0x03);

    (void)pt;
    (void)ct;

    /* 芯片不支持极性配置；电流/电压通道模式下 bit15 均需软件翻功率 */
    if (P3W4 != mode)
        return 0;

    if ((pCfgUser->base.PhaseAdjust1.value & 0x8000) == 0x8000)
        phase_A = -1;
    if ((pCfgUser->base.PhaseAdjust2.value & 0x8000) == 0x8000)
        phase_B = -1;
    if ((pCfgUser->base.PhaseAdjust3.value & 0x8000) == 0x8000)
        phase_C = -1;

    if ((phase_A == 1) && (phase_B == 1) && (phase_C == 1))
        return 0;

    out->Pa   = out->Pa * phase_A;
    out->Pb   = out->Pb * phase_B;
    out->Pc   = out->Pc * phase_C;
    out->Ptot = out->Pa + out->Pb + out->Pc;
    out->Qa   = out->Qa * phase_A;
    out->Qb   = out->Qb * phase_B;
    out->Qc   = out->Qc * phase_C;
    out->Qtot = out->Qa + out->Qb + out->Qc;
    out->PFa  = out->PFa * phase_A;
    out->PFb  = out->PFb * phase_B;
    out->PFc  = out->PFc * phase_C;
    if (out->Stot != 0)
        out->PFtot = (float)out->Ptot / out->Stot * 1000.0f;

    return 0;
}
