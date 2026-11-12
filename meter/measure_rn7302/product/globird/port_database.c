/**
 * @file        port_database.c
 * @brief       measure_rn7302 ↔ database 配置对接
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   端口数据库
 */
#include <stddef.h>
#include <stdint.h>
#include "port_database.h"

/**
 * @brief   分相 reg 换算：measure 的 krms=rated/reg/f 对齐 Globird 的 Vx/V/f
 *
 * rated 固定取 A 相 Vx_a；reg_x = V * Vx_a / Vx_x
 */
static uint32_t port_db_reg_from_phase(uint32_t v_common,
                                       uint32_t vx_a,
                                       uint32_t vx_x)
{
    if (vx_x == 0U || vx_a == 0U) {
        return v_common;
    }
    return (uint32_t)((uint64_t)v_common * (uint64_t)vx_a / (uint64_t)vx_x);
}

/**
 * @brief   将 database 回路配置拷贝到 measure 侧 msr_config
 *
 * 映射关系：
 * - PT/CT/接线/相序/脉冲 ← user.base / pulseout
 * - type_i0 ← sys.adjustI0value.TypeI0
 * - adjust ← sys.adjustAd* / adjustI0（rated/reg/factor 对齐 Globird Krms）
 * - deadzone ← sys.deadzone
 */
int port_db_copy_config_to_measure(struct msr_config *cfg,
                                   const struct db_database_measure_base_type *pMeasure)
{
    const struct db_cfg_user_para *pCfgUser;
    const struct db_cfg_sys_para *pCfgSys;

    if (cfg == NULL || pMeasure == NULL) {
        return -1;
    }

    pCfgUser = &pMeasure->db_cfg.user;
    pCfgSys  = &pMeasure->db_cfg.sys;

    cfg->user.pt              = pCfgUser->base.PT.value;
    cfg->user.ct              = pCfgUser->base.CT.value;
    cfg->user.wiremode        = pCfgUser->base.WiringMode.value;
    cfg->user.type_i0         = (uint16_t)pCfgSys->adjustI0value.TypeI0.value;
    cfg->user.pulse_constant  = pCfgUser->pulseout.pulse[0].constant.value;
    cfg->user.phase_adjust[0] = pCfgUser->base.PhaseAdjust1.value;
    cfg->user.phase_adjust[1] = pCfgUser->base.PhaseAdjust2.value;
    cfg->user.phase_adjust[2] = pCfgUser->base.PhaseAdjust3.value;

    /* rated←A 相寄存器目标；reg←合用额定并按相折算 */
    cfg->sys.adjust.rated_voltage = pCfgSys->adjustAdvalue.Adjust_V.value;
    cfg->sys.adjust.rated_current = pCfgSys->adjustAdvalue.Adjust_I.value;
    cfg->sys.adjust.rated_power   = pCfgSys->adjustAdvalue.Adjust_P.value;
    cfg->sys.adjust.rated_angle   = pCfgSys->adjustAdvalue.Adjust_A.value;

    cfg->sys.adjust.reg_va = pCfgSys->adjustAdvalue.Adjust_Va.value;
    cfg->sys.adjust.reg_vb = pCfgSys->adjustAdvalue.Adjust_Vb.value;
    cfg->sys.adjust.reg_vc = pCfgSys->adjustAdvalue.Adjust_Vc.value;
    cfg->sys.adjust.reg_vn = 1;
    cfg->sys.adjust.reg_ia = pCfgSys->adjustAdvalue.Adjust_Ia.value;
    cfg->sys.adjust.reg_ib = pCfgSys->adjustAdvalue.Adjust_Ib.value;
    cfg->sys.adjust.reg_ic = pCfgSys->adjustAdvalue.Adjust_Ic.value;
    cfg->sys.adjust.reg_in = pCfgSys->adjustI0value.Adjust_I0.value;
    cfg->sys.adjust.reg_pa = pCfgSys->adjustAdvalue.Adjust_Pa.value;
    cfg->sys.adjust.reg_pb = pCfgSys->adjustAdvalue.Adjust_Pb.value;
    cfg->sys.adjust.reg_pc = pCfgSys->adjustAdvalue.Adjust_Pc.value;

    cfg->sys.adjust.factor_voltage     = pCfgSys->adjustAd.Krms_V.value;
    cfg->sys.adjust.factor_current     = pCfgSys->adjustAd.Krms_I.value;
    cfg->sys.adjust.factor_power       = pCfgSys->adjustAd.Krms_P.value;
    cfg->sys.adjust.factor_angle       = 1;
    cfg->sys.adjust.factor_freq        = pCfgSys->adjustAd.Krms_Hz.value;
    cfg->sys.adjust.factor_energy      = pCfgSys->adjustAd.Krms_E.value;
    cfg->sys.adjust.factor_powerfactor = pCfgSys->adjustAd.Krms_Pf.value;

    cfg->sys.deadzone.va = pCfgSys->deadzone.Ua.value;
    cfg->sys.deadzone.vb = pCfgSys->deadzone.Ub.value;
    cfg->sys.deadzone.vc = pCfgSys->deadzone.Uc.value;
    cfg->sys.deadzone.vn = 0;
    cfg->sys.deadzone.ia = pCfgSys->deadzone.Ia.value;
    cfg->sys.deadzone.ib = pCfgSys->deadzone.Ib.value;
    cfg->sys.deadzone.ic = pCfgSys->deadzone.Ic.value;
    cfg->sys.deadzone.in = pCfgSys->deadzone.In.value;
    cfg->sys.deadzone.pa = pCfgSys->deadzone.Pa.value;
    cfg->sys.deadzone.pb = pCfgSys->deadzone.Pb.value;
    cfg->sys.deadzone.pc = pCfgSys->deadzone.Pc.value;
    cfg->sys.deadzone.qa = pCfgSys->deadzone.Qa.value;
    cfg->sys.deadzone.qb = pCfgSys->deadzone.Qb.value;
    cfg->sys.deadzone.qc = pCfgSys->deadzone.Qc.value;
    cfg->sys.deadzone.sa = pCfgSys->deadzone.Sa.value;
    cfg->sys.deadzone.sb = pCfgSys->deadzone.Sb.value;
    cfg->sys.deadzone.sc = pCfgSys->deadzone.Sc.value;

    cfg->sys.chip.HFConst1  = pCfgSys->adjust.HfConst1.value;
    cfg->sys.chip.HFConst2  = pCfgSys->adjust.HfConst2.value;
    // cfg->sys.chip.IStart_PS = 0; /* 无需赋值启动参数 */
    // cfg->sys.chip.IStart_Q  = 0; /* 无需赋值启动参数 */
    cfg->sys.chip.GSUA      = pCfgSys->adjust.GsUa.value;
    cfg->sys.chip.GSUB      = pCfgSys->adjust.GsUb.value;
    cfg->sys.chip.GSUC      = pCfgSys->adjust.GsUc.value;
    cfg->sys.chip.GSIA      = pCfgSys->adjust.GsIa.value;
    cfg->sys.chip.GSIB      = pCfgSys->adjust.GsIb.value;
    cfg->sys.chip.GSIC      = pCfgSys->adjust.GsIc.value;
    cfg->sys.chip.GSIN      = pCfgSys->adjust.GsIn.value;
    cfg->sys.chip.PHSUA     = pCfgSys->adjust.Phs_Ua.value;
    cfg->sys.chip.PHSUB     = pCfgSys->adjust.Phs_Ub.value;
    cfg->sys.chip.PHSUC     = pCfgSys->adjust.Phs_Uc.value;
    cfg->sys.chip.PHSIA     = pCfgSys->adjust.Phs_Ia.value;
    cfg->sys.chip.PHSIB     = pCfgSys->adjust.Phs_Ib.value;
    cfg->sys.chip.PHSIC     = pCfgSys->adjust.Phs_Ic.value;
    cfg->sys.chip.PHSIN     = pCfgSys->adjust.Phs_In.value;
    cfg->sys.chip.PA_PHSL   = pCfgSys->adjust.Pa_PhsL.value;
    cfg->sys.chip.PB_PHSL   = pCfgSys->adjust.Pb_PhsL.value;
    cfg->sys.chip.PC_PHSL   = pCfgSys->adjust.Pc_PhsL.value;
    cfg->sys.chip.QA_PHSL   = pCfgSys->adjust.Qa_PhsL.value;
    cfg->sys.chip.QB_PHSL   = pCfgSys->adjust.Qb_PhsL.value;
    cfg->sys.chip.QC_PHSL   = pCfgSys->adjust.Qc_PhsL.value;

    return 0;
}

/**
 * @brief   msr_data_realtime → db_basic_realtime_type
 */
static void port_db_copy_realtime(struct db_basic_realtime_type *dst,
                                  const struct msr_data_realtime *src)
{
    dst->Va     = src->Va;
    dst->Vb     = src->Vb;
    dst->Vc     = src->Vc;
    dst->Vn     = src->Vn;
    dst->VlnAvg = src->VlnAvg;
    dst->Vab    = src->Vab;
    dst->Vbc    = src->Vbc;
    dst->Vca    = src->Vca;
    dst->VllAvg = src->VllAvg;
    dst->Ia     = src->Ia;
    dst->Ib     = src->Ib;
    dst->Ic     = src->Ic;
    dst->In     = src->In;
    dst->IAvg   = src->Iavg;
    dst->Pa     = src->Pa;
    dst->Pb     = src->Pb;
    dst->Pc     = src->Pc;
    dst->Ptot   = src->Ptot;
    dst->Qa     = src->Qa;
    dst->Qb     = src->Qb;
    dst->Qc     = src->Qc;
    dst->Qtot   = src->Qtot;
    dst->Sa     = src->Sa;
    dst->Sb     = src->Sb;
    dst->Sc     = src->Sc;
    dst->Stot   = src->Stot;
    dst->PFa    = src->PFa;
    dst->PFb    = src->PFb;
    dst->PFc    = src->PFc;
    dst->PFtot  = src->PFtot;
    dst->Hz_hp  = src->Freq;
    dst->Hz     = src->Freq / 100;
    dst->J_Va   = src->AngleUa;
    dst->J_Vb   = src->AngleUb;
    dst->J_Vc   = src->AngleUc;
    dst->J_Ia   = src->AngleIa;
    dst->J_Ib   = src->AngleIb;
    dst->J_Ic   = src->AngleIc;
}

/**
 * @brief   将 measure 电参结果写回 database
 */
int port_db_copy_data_to_database(const struct msr_data *data,
                                  struct db_database_measure_base_type *pMeasure)
{
    if (data == NULL || pMeasure == NULL) {
        return -1;
    }

    port_db_copy_realtime(&pMeasure->db_measure.numeric.fullwave.primary, &data->realtime_full);
    port_db_copy_realtime(&pMeasure->db_measure.numeric.fundwave.primary, &data->realtime_fund);
    port_db_copy_realtime(&pMeasure->db_measure.numeric.halfwave.primary, &data->realtime_half);

    return 0;
}
