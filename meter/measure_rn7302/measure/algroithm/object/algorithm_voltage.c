/**
 * @file        algorithm_voltage.c
 * @brief       电压计算
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电压计算
 */
#include <stdlib.h>
#include "algorithm_voltage.h"


/**
 * @brief   死区约束：|value| < deadzone 则返回 0，否则原值
 *
 * @param[in] value     待约束值
 * @param[in] deadzone  死区阈值
 *
 * @return 约束后的值
 */
static int64_t volt_calc_range_constaraint(int64_t value, uint64_t deadzone)
{
    if (llabs(value) < (int64_t)deadzone)
    {
        return 0;
    }

    return value;
}

/**
 * @brief   全波/基波单相电压：寄存器 → 一次侧有效值
 *
 * temp = emu×pt/Krms，再按 deadzone×pt 死区，最后 /MULTIPLE_PT。
 *
 * @param[in] emu_u          芯片电压有效值寄存器
 * @param[in] rated_voltage  额定电压（校表）
 * @param[in] reg_vx         校表寄存器目标
 * @param[in] factor_voltage 精度系数
 * @param[in] deadzone_vx    死区
 * @param[in] pt             PT
 * @return 一次侧电压（×100 刻度一类，与 Globird 一致）
 */
static int32_t volt_fullfund_phase_single(int32_t emu_u,
                                          uint32_t rated_voltage, 
                                          uint32_t reg_vx, 
                                          uint32_t factor_voltage, 
                                          uint32_t deadzone_vx, 
                                          uint32_t pt)
{
    uint32_t temp = 0;
    uint32_t krms = rated_voltage / reg_vx / factor_voltage;
    uint64_t dead = deadzone_vx * pt;

    if (krms <= 0) {
        krms = RN7302_Krmsu;
    }

    temp = (uint32_t)((uint64_t)emu_u * pt / krms);
    temp = (uint32_t)volt_calc_range_constaraint(temp, dead);

    return temp / MULTIPLE_PT;
}

/**
 * @brief   全波/基波相电压：按接线写 Va/Vb/Vc/VlnAvg
 *
 * @param[out] pDataRt  写出相电压
 * @param[in]  pCfg     接线、PT、校表、死区
 * @param[in]  pIn      含 UA/UB/UC
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t voltage_fullfund_phase(struct msr_data_realtime *pDataRt,
                                      const struct msr_config *pCfg,
                                      const struct msr_input_fullfund *pIn)
{
    const struct cfg_adjust   *pAdjust = &pCfg->sys.adjust; /* 校准系数相关 */
    const struct cfg_deadzone *pDead   = &pCfg->sys.deadzone; /* 死区相关 */

    switch (pCfg->user.wiremode)
    {
    case WIREMODE_P1W2:
        pDataRt->Va = volt_fullfund_phase_single(pIn->UA, pAdjust->rated_voltage, 
            pAdjust->reg_va, pAdjust->factor_voltage, pDead->va, pCfg->user.pt);
        pDataRt->Vb = 0;
        pDataRt->Vc = 0;
        pDataRt->VlnAvg = (pDataRt->Va + pDataRt->Vb + pDataRt->Vc) / 3;
        break;
    case WIREMODE_P3W3:
        pDataRt->Va     = 0;
        pDataRt->Vb     = 0;
        pDataRt->Vc     = 0;
        pDataRt->VlnAvg = 0;
        break;
    case WIREMODE_P3W4:
        pDataRt->Va = volt_fullfund_phase_single(pIn->UA, pAdjust->rated_voltage, 
            pAdjust->reg_va, pAdjust->factor_voltage, pDead->va, pCfg->user.pt);
        pDataRt->Vb = volt_fullfund_phase_single(pIn->UB, pAdjust->rated_voltage, 
            pAdjust->reg_vb, pAdjust->factor_voltage, pDead->vb, pCfg->user.pt);
        pDataRt->Vc = volt_fullfund_phase_single(pIn->UC, pAdjust->rated_voltage, 
            pAdjust->reg_vc, pAdjust->factor_voltage, pDead->vc, pCfg->user.pt);
        pDataRt->VlnAvg = (pDataRt->Va + pDataRt->Vb + pDataRt->Vc) / 3;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   P3W4 线电压：余弦定理 rms = sqrt(v1²+v2²-2·v1·v2·cosφ)
 *
 * @return 线电压有效值（已做死区）
 */
static uint32_t volt_fullfund_line_calc_p3w4(double v1, double v2, double pha_rad, uint64_t deadzone)
{
    uint32_t rms = 0;
    double s = v1 * v1 + v2 * v2 - 2 * v1 * v2 * cos(pha_rad);

    if (s < 0.0f) {
        s = -s;
    }

    rms = (uint32_t)sqrt(s);
    rms = (uint32_t)volt_calc_range_constaraint(rms, deadzone);

    return rms;
}

/**
 * @brief   P3W3：已知 Uab/Ubc 与夹角 radB，求 Uca 及相对 Uab 的相位
 *
 * @param[in]  uab, ubc   两线电压有效值
 * @param[in]  radB       Uab–Ubc 夹角（弧度）
 * @param[out] voltage    [0]=Uab [1]=Ubc [2]=Uca
 * @param[out] pha_rad    相对 Uab 的三相相位（弧度）
 */
static void volt_fullfund_line_calc_p3w3(float uab, float ubc, float radB, uint32_t voltage[3], double pha_rad[3])
{
    /* 计算线电压 uab/ubc（传参已知）、uca */
    voltage[0] = uab;
    voltage[1] = ubc;
    voltage[2] = sqrt(uab * uab + ubc * ubc - 2 * uab * ubc * cos(radB)); /* 已知两边一夹角，求第三边 */
    float uca = voltage[2];

    /* 计算三角形各夹角 radA、radB（传参已知）、radC */
    float radC = asin(ubc * sin(radB) / uca);       /* 正弦定理：已知两边一夹角，另外一角 */
    // float radA = PI - radC - radB;                  /* 已知三角形两角，用内角和等于180°求第三角 */

    /* 
        将三角形内角转换成基于uab的相位角：

              A(∠A)
             / \
            /   \
    Uca    /     \    Uab
          /       \
         /         \
        /           \
   (∠C)C------------B(∠B)
           Ubc
    */
    pha_rad[0] = 0;
    pha_rad[1] = 2 * PI - (PI - radB);  /* 将radB翻转180°(PI - radB)，再逆时针得到radB基于uab的相位角 */
    pha_rad[2] = PI - radC;             /* 将radC翻转180°(PI - radC) */
}

/**
 * @brief   芯片相位寄存器 → 0.01°，范围 [0, 36000)
 */
static int32_t angle_emu_calc(int32_t emu_y)
{
    uint32_t val = (uint32_t)ANGLE_CONVERT(emu_y);

    return (int32_t)((ANGLE_J360_MAX - val) % ANGLE_J360_MAX);
}

/**
 * @brief   全波/基波线电压：按接线写 Vab/Vbc/Vca/VllAvg
 *
 * P3W4 依赖 pDataRt 中已有 Angle*；P3W3 用 UA/UC/YUC 合成并写 Angle*。
 *
 * @param[in,out] pDataRt  读写电压与必要时相角
 * @param[in]     pCfg     接线、PT、校表、死区
 * @param[in]     pIn      电参快照
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t voltage_fullfund_line(struct msr_data_realtime *pDataRt,
                                     const struct msr_config *pCfg,
                                     const struct msr_input_fullfund *pIn)
{
    uint32_t voltage[3] = {0};
    double   pha_rad[3] = {0};

    const struct cfg_adjust   *pAdjust = &pCfg->sys.adjust; /* 校准系数相关 */
    const struct cfg_deadzone *pDead   = &pCfg->sys.deadzone; /* 死区相关 */

    switch (pCfg->user.wiremode)
    {
    case WIREMODE_P1W2:
        pDataRt->Vab    = 0;
        pDataRt->Vbc    = 0;
        pDataRt->Vca    = 0;
        pDataRt->VllAvg = 0;
        break;
    case WIREMODE_P3W3:
        uint32_t uab = volt_fullfund_phase_single(pIn->UA, pAdjust->rated_voltage, 
            pAdjust->reg_va, pAdjust->factor_voltage, pDead->va, pCfg->user.pt);
        uint32_t ubc = volt_fullfund_phase_single(pIn->UC, pAdjust->rated_voltage, 
            pAdjust->reg_vc, pAdjust->factor_voltage, pDead->vc, pCfg->user.pt);
        float radB = (float)angle_emu_calc(pIn->YUC) * PI / 18000;

        volt_fullfund_line_calc_p3w3(uab, ubc, radB, voltage, pha_rad);
        
        pDataRt->AngleUa = pha_rad[0] * 18000 / PI;
        pDataRt->AngleUb = pha_rad[1] * 18000 / PI;
        pDataRt->AngleUc = pha_rad[2] * 18000 / PI;

        pDataRt->Vab = voltage[0];
        pDataRt->Vbc = voltage[1];
        pDataRt->Vca = voltage[2];
        pDataRt->VllAvg = (pDataRt->Vab + pDataRt->Vbc + pDataRt->Vca) / 3;
        break;
    case WIREMODE_P3W4:
        pha_rad[0] = (pDataRt->AngleUa - pDataRt->AngleUb) * PI / 18000;
        pha_rad[1] = (pDataRt->AngleUb - pDataRt->AngleUc) * PI / 18000;
        pha_rad[2] = (pDataRt->AngleUc - pDataRt->AngleUa) * PI / 18000;
        pDataRt->Vab = volt_fullfund_line_calc_p3w4(pDataRt->Va, pDataRt->Vb, pha_rad[0], 
            pDead->va * pCfg->user.pt / MULTIPLE_PT);
        pDataRt->Vbc = volt_fullfund_line_calc_p3w4(pDataRt->Vb, pDataRt->Vc, pha_rad[1], 
            pDead->vb * pCfg->user.pt / MULTIPLE_PT);
        pDataRt->Vca = volt_fullfund_line_calc_p3w4(pDataRt->Vc, pDataRt->Va, pha_rad[2], 
            pDead->vc * pCfg->user.pt / MULTIPLE_PT);
        pDataRt->VllAvg = (pDataRt->Vab + pDataRt->Vbc + pDataRt->Vca) / 3;
        break;
    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   全波/基波电压（相电压 + 线电压）
 *
 * P3W4 线电压用 out 中已有 Angle*（调度须先调 alg_voltageangle）；
 * P3W3 由 UA/UC 与 YUC 三角合成线电压并写回 Angle*。
 *
 * @param[out] out  Va…/Vab… 及必要时 AngleU*
 * @param[in]  cfg  接线、PT、校表、死区
 * @param[in]  in   电参快照（UA/UB/UC、YUC 等）
 *
 * @return 0 成功；参数为空返回 -1
 */
int32_t algorithm_voltage_fullfund(struct msr_data_realtime *out,
                                   const struct msr_config *cfg,
                                   const struct msr_input_fullfund *in)
{
    if (out == NULL || cfg == NULL || in == NULL) {
        return -1;
    }

    voltage_fullfund_phase(out, cfg, in);
    voltage_fullfund_line (out, cfg, in);

    return 0;
}

/**
 * @brief   由半波环缓冲两槽合成有效值（不写槽）
 *
 * 与 Globird 一致：sqrt(sum×65536/N)×256/kf，再乘 PT/Krms 并死区。
 *
 * @param[in] ring            两槽原始寄存器值
 * @param[in] kf              频率修正系数
 * @param[in] rated_voltage   额定电压
 * @param[in] reg_vx          校表寄存器目标
 * @param[in] factor_voltage  精度系数
 * @param[in] deadzone_vx     死区
 * @param[in] pt              PT
 *
 * @return 一次侧半波电压有效值
 */
static uint32_t volt_halfwave_rms_of_ring(const int32_t *ring,
                                         float kf,
                                         uint32_t rated_voltage,
                                         uint32_t reg_vx,
                                         uint32_t factor_voltage,
                                         uint32_t deadzone_vx,
                                         uint32_t pt)
{
    uint64_t temp = 0;
    uint64_t dead;
    uint32_t krms;
    uint32_t j;

    krms = rated_voltage / reg_vx / factor_voltage;
    if (krms == 0) {
        krms = RN7302_Krmsu;
    }

    for (j = 0; j < MSR_HALFWAVE_SLOTS; j++) {
        temp += (uint64_t)(uint32_t)ring[j];
    }

    temp = (uint64_t)sqrt((double)(temp * 65536u / MSR_HALFWAVE_SLOTS)) * 256u / kf;
    temp = temp * pt / MULTIPLE_PT / krms;
    dead = (uint64_t)deadzone_vx * pt / MULTIPLE_PT;
    if (temp < dead) {
        temp = 0;
    }

    return (uint32_t)temp;
}

/**
 * @brief   半波单相有效值：写入环槽后换算
 *
 * @param[in]     emu_u   本拍芯片半波寄存器
 * @param[in,out] ring    两槽环缓冲
 * @param[in]     slot    本拍写入槽索引
 * @param[in]     kf      频率修正系数
 * @param[in]     rated_voltage / reg_vx / factor_voltage / deadzone_vx / pt  校表与变比
 *
 * @return 一次侧半波电压有效值
 */
static uint32_t volt_halfwave_phase_single(int32_t emu_u,
                                           int32_t *ring,
                                           uint32_t slot,
                                           float kf,
                                           uint32_t rated_voltage,
                                           uint32_t reg_vx,
                                           uint32_t factor_voltage,
                                           uint32_t deadzone_vx,
                                           uint32_t pt)
{
    ring[slot] = emu_u;
    return volt_halfwave_rms_of_ring(ring, kf, rated_voltage, reg_vx,
                                     factor_voltage, deadzone_vx, pt);
}

/**
 * @brief   半波相电压（按接线模式）
 *
 * P1W2：只算 Va；P3W3：相电压置 0（线电压在 line 中由 UA/UC 半波有效值合成）；
 * P3W4：三相。各模式均更新环缓冲，供 line / 跨拍使用。
 *
 * @param[out]    out    Va/Vb/Vc/VlnAvg
 * @param[in]     cfg    接线、PT、校表、死区
 * @param[in]     in     半波 UA/UB/UC
 * @param[in,out] st     环缓冲与 wr（本函数递增 wr）
 * @param[in]     hz_hp  频率×10000
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t voltage_halfwave_phase(struct msr_data_realtime *out,
                                      const struct msr_config *cfg,
                                      const struct msr_input_fullfund *in,
                                      struct msr_halfwave_volt_state *st,
                                      uint32_t hz_hp)
{
    const struct cfg_adjust   *pAdjust = &cfg->sys.adjust;
    const struct cfg_deadzone *pDead   = &cfg->sys.deadzone;
    uint32_t slot = st->wr % MSR_HALFWAVE_SLOTS;
    float kf = sqrtf((float)(4000 * 10000) / (float)hz_hp);
    uint32_t va, vb, vc;

    /* 始终写入三路环，保证 3p3w 线电压与跨拍状态完整 */
    va = volt_halfwave_phase_single(in->UA, st->ua, slot, kf,
        pAdjust->rated_voltage, pAdjust->reg_va, pAdjust->factor_voltage,
        pDead->va, cfg->user.pt);
    vb = volt_halfwave_phase_single(in->UB, st->ub, slot, kf,
        pAdjust->rated_voltage, pAdjust->reg_vb, pAdjust->factor_voltage,
        pDead->vb, cfg->user.pt);
    vc = volt_halfwave_phase_single(in->UC, st->uc, slot, kf,
        pAdjust->rated_voltage, pAdjust->reg_vc, pAdjust->factor_voltage,
        pDead->vc, cfg->user.pt);

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Va = va;
        out->Vb = 0;
        out->Vc = 0;
        out->VlnAvg = (out->Va + out->Vb + out->Vc) / 3;
        break;
    case WIREMODE_P3W3:
        out->Va = 0;
        out->Vb = 0;
        out->Vc = 0;
        out->VlnAvg = 0;
        break;
    case WIREMODE_P3W4:
        out->Va = va;
        out->Vb = vb;
        out->Vc = vc;
        out->VlnAvg = (uint32_t)(((uint64_t)out->Va + out->Vb + out->Vc) / 3);
        break;
    default:
        return -1;
    }

    st->wr++;
    return 0;
}

/**
 * @brief   半波线电压（按接线模式）
 *
 * P1W2：线电压清零；P3W3：UA/UC 半波有效值作线电压，三角合成 Vca 与相角；
 * P3W4：用已有相电压与 Angle* 余弦定理（调度须先写相角）。
 *
 * @param[in,out] out    读写线电压；P3W3 写 Angle*；P3W4 读相电压与 Angle*
 * @param[in]     cfg    接线、PT、校表、死区
 * @param[in]     in     半波快照（P3W3 用 YUC）
 * @param[in]     st     已写入本拍的环缓冲（只读）
 * @param[in]     hz_hp  频率×10000
 *
 * @return 0 成功；接线非法返回 -1
 */
static int32_t voltage_halfwave_line(struct msr_data_realtime *out,
                                     const struct msr_config *cfg,
                                     const struct msr_input_fullfund *in,
                                     const struct msr_halfwave_volt_state *st,
                                     uint32_t hz_hp)
{
    const struct cfg_adjust   *pAdjust = &cfg->sys.adjust;
    const struct cfg_deadzone *pDead   = &cfg->sys.deadzone;
    uint32_t pt = cfg->user.pt;
    uint32_t voltage[3] = {0};
    double pha_rad[3] = {0};
    float kf;
    uint32_t uab, ubc;
    float radB;

    switch (cfg->user.wiremode) {
    case WIREMODE_P1W2:
        out->Vab = 0;
        out->Vbc = 0;
        out->Vca = 0;
        out->VllAvg = 0;
        break;

    case WIREMODE_P3W3:
        kf = sqrtf((float)(4000 * 10000) / (float)hz_hp);
        uab = volt_halfwave_rms_of_ring(st->ua, kf,
            pAdjust->rated_voltage, pAdjust->reg_va, pAdjust->factor_voltage,
            pDead->va, cfg->user.pt);
        ubc = volt_halfwave_rms_of_ring(st->uc, kf,
            pAdjust->rated_voltage, pAdjust->reg_vc, pAdjust->factor_voltage,
            pDead->vc, cfg->user.pt);
        radB = (float)angle_emu_calc(in->YUC) * PI / 18000;

        volt_fullfund_line_calc_p3w3(uab, ubc, radB, voltage, pha_rad);

        out->AngleUa = (int16_t)(pha_rad[0] * 18000 / PI);
        out->AngleUb = (int16_t)(pha_rad[1] * 18000 / PI);
        out->AngleUc = (int16_t)(pha_rad[2] * 18000 / PI);
        out->Vab = voltage[0];
        out->Vbc = voltage[1];
        out->Vca = voltage[2];
        out->VllAvg = (uint32_t)(((uint64_t)out->Vab + out->Vbc + out->Vca) / 3);
        break;

    case WIREMODE_P3W4:
        pha_rad[0] = (out->AngleUa - out->AngleUb) * PI / 18000;
        pha_rad[1] = (out->AngleUb - out->AngleUc) * PI / 18000;
        pha_rad[2] = (out->AngleUc - out->AngleUa) * PI / 18000;
        out->Vab = volt_fullfund_line_calc_p3w4(out->Va, out->Vb, pha_rad[0],
            (uint64_t)pDead->va * pt / MULTIPLE_PT);
        out->Vbc = volt_fullfund_line_calc_p3w4(out->Vb, out->Vc, pha_rad[1],
            (uint64_t)pDead->vb * pt / MULTIPLE_PT);
        out->Vca = volt_fullfund_line_calc_p3w4(out->Vc, out->Va, pha_rad[2],
            (uint64_t)pDead->vc * pt / MULTIPLE_PT);
        out->VllAvg = (uint32_t)(((uint64_t)out->Vab + out->Vbc + out->Vca) / 3);
        break;

    default:
        return -1;
    }

    return 0;
}

/**
 * @brief   半波电压单入口（相电压 + 线电压，按 cfg->wiremode 分支）
 *
 * 环缓冲与写槽由 st 持有；hz_hp 为频率×10000。
 * - P1W2：只出 Va；线电压清零
 * - P3W3：相电压清零；线电压由 UA/UC 半波有效值三角合成，并写回 Angle*
 * - P3W4：三相相电压；线电压用已有 Angle*，调度须先写相角：
 *     voltageangle → alg_halfwave_voltage → …
 *
 * @param[out]    out    Va/Vb/Vc/VlnAvg 与 Vab/Vbc/Vca/VllAvg
 * @param[in]     cfg    接线、PT、校表、死区
 * @param[in]     in     半波电参快照（UA/UB/UC 为半波寄存器）
 * @param[in,out] st     跨拍原始值环与 wr
 * @param[in]     hz_hp  频率（×10000）
 *
 * @return 0 成功；hz_hp==0、参数为空或接线非法返回 -1
 */
int32_t algorithm_halfwave_voltage(struct msr_data_realtime *out,
                                   const struct msr_config *cfg,
                                   const struct msr_input_fullfund *in,
                                   struct msr_halfwave_volt_state *st,
                                   uint32_t hz_hp)
{
    int32_t ret;

    if (out == NULL || cfg == NULL || in == NULL || st == NULL || hz_hp == 0) {
        return -1;
    }

    ret = voltage_halfwave_phase(out, cfg, in, st, hz_hp);
    if (ret != 0) {
        return ret;
    }

    return voltage_halfwave_line(out, cfg, in, st, hz_hp);
}
