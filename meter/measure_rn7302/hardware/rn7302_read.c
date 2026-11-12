/**
 * @file        rn7302_read.c
 * @brief       RN7302 读取
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302 读取
 *  2026-10-29  yansh    1.0.1   fond→fund；半波写入 half
 */
#include <stddef.h>
#include <stdint.h>
#include "rn7302_read.h"
#include "rn7302_register.h"
#include "port_rn7302.h"

/**
 * @brief       RN7302 读取全波寄存器
 * @param[in]   pIn    输入结构体
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_full(struct msr_input *pIn)
{
    if (pIn == NULL) {
        return -1;
    }

    if ((0 != MeterChipReadReg(RN7302_REG_UA,    RN7302_LEN_UA,     &pIn->full.UA   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_UB,    RN7302_LEN_UB,     &pIn->full.UB   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_UC,    RN7302_LEN_UC,     &pIn->full.UC   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_IA,    RN7302_LEN_IA,     &pIn->full.IA   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_IB,    RN7302_LEN_IB,     &pIn->full.IB   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_IC,    RN7302_LEN_IC,     &pIn->full.IC   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_IN,    RN7302_LEN_IN,     &pIn->full.IN   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PA,    RN7302_LEN_PA,     &pIn->full.PA   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PB,    RN7302_LEN_PB,     &pIn->full.PB   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PC,    RN7302_LEN_PC,     &pIn->full.PC   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PT,    RN7302_LEN_PT,     &pIn->full.PT   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_QA,    RN7302_LEN_QA,     &pIn->full.QA   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_QB,    RN7302_LEN_QB,     &pIn->full.QB   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_QC,    RN7302_LEN_QC,     &pIn->full.QC   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_QT,    RN7302_LEN_QT,     &pIn->full.QT   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_SA,    RN7302_LEN_SA,     &pIn->full.SA   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_SB,    RN7302_LEN_SB,     &pIn->full.SB   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_SC,    RN7302_LEN_SC,     &pIn->full.SC   ))
    ||  (0 != MeterChipReadReg(RN7302_REG_STA,   RN7302_LEN_STA,    &pIn->full.STA  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PfA,   RN7302_LEN_PfA,    &pIn->full.PfA  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PfB,   RN7302_LEN_PfB,    &pIn->full.PfB  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PfC,   RN7302_LEN_PfC,    &pIn->full.PfC  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_PfTV,  RN7302_LEN_PfTV,   &pIn->full.PfTV ))
    ||  (0 != MeterChipReadReg(RN7302_REG_UFreq, RN7302_LEN_UFreq,  &pIn->full.UFreq))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUA,   RN7302_LEN_YUA,    &pIn->full.YUA  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUB,   RN7302_LEN_YUB,    &pIn->full.YUB  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUC,   RN7302_LEN_YUC,    &pIn->full.YUC  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIA,   RN7302_LEN_YIA,    &pIn->full.YIA  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIB,   RN7302_LEN_YIB,    &pIn->full.YIB  ))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIC,   RN7302_LEN_YIC,    &pIn->full.YIC  ))) {
        return -1;
    }

    return 0;
}

/**
 * @brief       RN7302 读取基波寄存器
 * @param[in]   pIn    输入结构体
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_fund(struct msr_input *pIn)
{
    if (pIn == NULL) {
        return -1;
    }

    if ((0 != MeterChipReadReg(RN7302_REG_FUA,   RN7302_LEN_FUA,    &pIn->fund.UA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FUB,   RN7302_LEN_FUB,    &pIn->fund.UB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FUC,   RN7302_LEN_FUC,    &pIn->fund.UC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FIA,   RN7302_LEN_FIA,    &pIn->fund.IA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FIB,   RN7302_LEN_FIB,    &pIn->fund.IB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FIC,   RN7302_LEN_FIC,    &pIn->fund.IC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPA,   RN7302_LEN_FPA,    &pIn->fund.PA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPB,   RN7302_LEN_FPB,    &pIn->fund.PB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPC,   RN7302_LEN_FPC,    &pIn->fund.PC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPT,   RN7302_LEN_FPT,    &pIn->fund.PT))
    ||  (0 != MeterChipReadReg(RN7302_REG_FQA,   RN7302_LEN_FQA,    &pIn->fund.QA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FQB,   RN7302_LEN_FQB,    &pIn->fund.QB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FQC,   RN7302_LEN_FQC,    &pIn->fund.QC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FQT,   RN7302_LEN_FQT,    &pIn->fund.QT))
    ||  (0 != MeterChipReadReg(RN7302_REG_FSA,   RN7302_LEN_FSA,    &pIn->fund.SA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FSB,   RN7302_LEN_FSB,    &pIn->fund.SB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FSC,   RN7302_LEN_FSC,    &pIn->fund.SC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FSTA,  RN7302_LEN_FSTA,   &pIn->fund.STA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPfA,  RN7302_LEN_FPfA,   &pIn->fund.PfA))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPfB,  RN7302_LEN_FPfB,   &pIn->fund.PfB))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPfC,  RN7302_LEN_FPfC,   &pIn->fund.PfC))
    ||  (0 != MeterChipReadReg(RN7302_REG_FPfTV, RN7302_LEN_FPfTV,  &pIn->fund.PfTV))
    ||  (0 != MeterChipReadReg(RN7302_REG_UFreq, RN7302_LEN_UFreq,  &pIn->fund.UFreq))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUA,   RN7302_LEN_YUA,    &pIn->fund.YUA))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUB,   RN7302_LEN_YUB,    &pIn->fund.YUB))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUC,   RN7302_LEN_YUC,    &pIn->fund.YUC))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIA,   RN7302_LEN_YIA,    &pIn->fund.YIA))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIB,   RN7302_LEN_YIB,    &pIn->fund.YIB))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIC,   RN7302_LEN_YIC,    &pIn->fund.YIC))) {
        return -1;
    }

    return 0;
}

/**
 * @brief       RN7302 读取半波寄存器
 * @param[in]   pIn    输入结构体
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_msr_half(struct msr_input *pIn)
{
    uint32_t value_ui[8] = {0};
    uint32_t value_pq[8] = {0};

    if (pIn == NULL) {
        return -1;
    }

    if ((sizeof(value_ui) != MeterChipReadArea(RN7302_REG_UA_HW,  sizeof(value_ui), value_ui))
    ||  (sizeof(value_pq) != MeterChipReadArea(RN7302_REG_SPL_PA, sizeof(value_pq), value_pq))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUA,   RN7302_LEN_YUA,   &pIn->half.YUA))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUB,   RN7302_LEN_YUB,   &pIn->half.YUB))
    ||  (0 != MeterChipReadReg(RN7302_REG_YUC,   RN7302_LEN_YUC,   &pIn->half.YUC))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIA,   RN7302_LEN_YIA,   &pIn->half.YIA))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIB,   RN7302_LEN_YIB,   &pIn->half.YIB))
    ||  (0 != MeterChipReadReg(RN7302_REG_YIC,   RN7302_LEN_YIC,   &pIn->half.YIC))
    ||  (0 != MeterChipReadReg(RN7302_REG_UFreq, RN7302_LEN_UFreq, &pIn->half.UFreq))) {
        return -1;
    }

    pIn->half.UA = (int32_t)value_ui[0];
    pIn->half.UB = (int32_t)value_ui[1];
    pIn->half.UC = (int32_t)value_ui[2];
    pIn->half.IA = (int32_t)value_ui[3];
    pIn->half.IB = (int32_t)value_ui[4];
    pIn->half.IC = (int32_t)value_ui[5];

    pIn->half.PA = (int32_t)value_pq[0];
    pIn->half.PB = (int32_t)value_pq[1];
    pIn->half.PC = (int32_t)value_pq[2];
    pIn->half.QA = (int32_t)value_pq[3];
    pIn->half.QB = (int32_t)value_pq[4];
    pIn->half.QC = (int32_t)value_pq[5];

    return 0;
}

/**
 * @brief       RN7302 读取全波电能寄存器
 * @param[in]   pIn    输入结构体
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_eng_full(struct msr_input *pIn)
{
    if (pIn == NULL) {
        return -1;
    }

    MeterChipReadReg(RN7302_REG_PosEPA, RN7302_LEN_PosEPA, &pIn->full.energy.PosEPa);
    MeterChipReadReg(RN7302_REG_PosEPB, RN7302_LEN_PosEPB, &pIn->full.energy.PosEPb);
    MeterChipReadReg(RN7302_REG_PosEPC, RN7302_LEN_PosEPC, &pIn->full.energy.PosEPc);
    MeterChipReadReg(RN7302_REG_PosEPT, RN7302_LEN_PosEPT, &pIn->full.energy.PosEPt);
    MeterChipReadReg(RN7302_REG_NegEPA, RN7302_LEN_NegEPA, &pIn->full.energy.PosEPa);
    MeterChipReadReg(RN7302_REG_NegEPB, RN7302_LEN_NegEPB, &pIn->full.energy.PosEPb);
    MeterChipReadReg(RN7302_REG_NegEPC, RN7302_LEN_NegEPC, &pIn->full.energy.PosEPc);
    MeterChipReadReg(RN7302_REG_NegEPT, RN7302_LEN_NegEPT, &pIn->full.energy.PosEPt);
    MeterChipReadReg(RN7302_REG_EPT,    RN7302_LEN_EPT,    &pIn->full.energy.EPt);
    MeterChipReadReg(RN7302_REG_PosEQA, RN7302_LEN_PosEQA, &pIn->full.energy.PosEQa);
    MeterChipReadReg(RN7302_REG_PosEQB, RN7302_LEN_PosEQB, &pIn->full.energy.PosEQb);
    MeterChipReadReg(RN7302_REG_PosEQC, RN7302_LEN_PosEQC, &pIn->full.energy.PosEQc);
    MeterChipReadReg(RN7302_REG_PosEQT, RN7302_LEN_PosEQT, &pIn->full.energy.PosEQt);
    MeterChipReadReg(RN7302_REG_NegEQA, RN7302_LEN_NegEQA, &pIn->full.energy.PosEQa);
    MeterChipReadReg(RN7302_REG_NegEQB, RN7302_LEN_NegEQB, &pIn->full.energy.PosEQb);
    MeterChipReadReg(RN7302_REG_NegEQC, RN7302_LEN_NegEQC, &pIn->full.energy.PosEQc);
    MeterChipReadReg(RN7302_REG_NegEQT, RN7302_LEN_NegEQT, &pIn->full.energy.PosEQt);
    MeterChipReadReg(RN7302_REG_EQT,    RN7302_LEN_EQT,    &pIn->full.energy.EQt);
    MeterChipReadReg(RN7302_REG_ESA,    RN7302_LEN_ESA,    &pIn->full.energy.ESa);
    MeterChipReadReg(RN7302_REG_ESB,    RN7302_LEN_ESB,    &pIn->full.energy.ESb);
    MeterChipReadReg(RN7302_REG_ESC,    RN7302_LEN_ESC,    &pIn->full.energy.ESc);
    MeterChipReadReg(RN7302_REG_ESTA,   RN7302_LEN_ESTA,   &pIn->full.energy.ESt);
    MeterChipReadReg(RN7302_REG_ESTV,   RN7302_LEN_ESTV,   &pIn->full.energy.Estv);

    return 0;
}

/**
 * @brief       RN7302 读取基波电能寄存器
 * @param[in]   pIn    输入结构体
 * @return      0 成功；-1 失败
 */
int get_rn7302_register_eng_fund(struct msr_input *pIn)
{
    if (pIn == NULL) {
        return -1;
    }

    MeterChipReadReg(RN7302_REG_PosFEPA, RN7302_LEN_PosFEPA, &pIn->fund.energy.PosEPa);
    MeterChipReadReg(RN7302_REG_PosFEPB, RN7302_LEN_PosFEPB, &pIn->fund.energy.PosEPb);
    MeterChipReadReg(RN7302_REG_PosFEPC, RN7302_LEN_PosFEPC, &pIn->fund.energy.PosEPc);
    MeterChipReadReg(RN7302_REG_PosFEPT, RN7302_LEN_PosFEPT, &pIn->fund.energy.PosEPt);
    MeterChipReadReg(RN7302_REG_NegFEPA, RN7302_LEN_NegFEPA, &pIn->fund.energy.PosEPa);
    MeterChipReadReg(RN7302_REG_NegFEPB, RN7302_LEN_NegFEPB, &pIn->fund.energy.PosEPb);
    MeterChipReadReg(RN7302_REG_NegFEPC, RN7302_LEN_NegFEPC, &pIn->fund.energy.PosEPc);
    MeterChipReadReg(RN7302_REG_NegFEPT, RN7302_LEN_NegFEPT, &pIn->fund.energy.PosEPt);
    MeterChipReadReg(RN7302_REG_FEPT,    RN7302_LEN_FEPT,    &pIn->fund.energy.EPt);
    MeterChipReadReg(RN7302_REG_PosFEQA, RN7302_LEN_PosFEQA, &pIn->fund.energy.PosEQa);
    MeterChipReadReg(RN7302_REG_PosFEQB, RN7302_LEN_PosFEQB, &pIn->fund.energy.PosEQb);
    MeterChipReadReg(RN7302_REG_PosFEQC, RN7302_LEN_PosFEQC, &pIn->fund.energy.PosEQc);
    MeterChipReadReg(RN7302_REG_PosFEQT, RN7302_LEN_PosFEQT, &pIn->fund.energy.PosEQt);
    MeterChipReadReg(RN7302_REG_NegFEQA, RN7302_LEN_NegFEQA, &pIn->fund.energy.PosEQa);
    MeterChipReadReg(RN7302_REG_NegFEQB, RN7302_LEN_NegFEQB, &pIn->fund.energy.PosEQb);
    MeterChipReadReg(RN7302_REG_NegFEQC, RN7302_LEN_NegFEQC, &pIn->fund.energy.PosEQc);
    MeterChipReadReg(RN7302_REG_NegFEQT, RN7302_LEN_NegFEQT, &pIn->fund.energy.PosEQt);
    MeterChipReadReg(RN7302_REG_FEQT,    RN7302_LEN_FEQT,    &pIn->fund.energy.EQt);
    MeterChipReadReg(RN7302_REG_FESA,    RN7302_LEN_FESA,    &pIn->fund.energy.ESa);
    MeterChipReadReg(RN7302_REG_FESB,    RN7302_LEN_FESB,    &pIn->fund.energy.ESb);
    MeterChipReadReg(RN7302_REG_FESC,    RN7302_LEN_FESC,    &pIn->fund.energy.ESc);
    MeterChipReadReg(RN7302_REG_FESTA,   RN7302_LEN_FESTA,   &pIn->fund.energy.ESt);
    MeterChipReadReg(RN7302_REG_FESTV,   RN7302_LEN_FESTV,   &pIn->fund.energy.Estv);

    return 0;
}
