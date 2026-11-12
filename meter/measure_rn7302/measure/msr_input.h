/**
 * @file        msr_input.h
 * @brief       电参快照
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-27  yansh    1.0.0   电参快照
 *  2026-10-28  yansh    1.0.1   拆分 fullfund / energy 聚合
 */
#ifndef MSR_INPUT_H
#define MSR_INPUT_H

#include <stdint.h>


/**
 * 芯片电能增量快照（与 rn7302_emu_energy 对应字段）
 */
struct msr_input_energy
{
    /*" "  全波计量参数寄存器""*/
    int32_t    PosEPa;
    int32_t    PosEPb;
    int32_t    PosEPc;
    int32_t    PosEPt;
    int32_t    NegEPa;
    int32_t    NegEPb;
    int32_t    NegEPc;
    int32_t    NegEPt;
    int32_t    EPt;
    int32_t    PosEQa;
    int32_t    PosEQb;
    int32_t    PosEQc;
    int32_t    PosEQt;
    int32_t    NegEQa;
    int32_t    NegEQb;
    int32_t    NegEQc;
    int32_t    NegEQt;
    int32_t    EQt;
    int32_t    ESa;
    int32_t    ESb;
    int32_t    ESc;
    int32_t    ESt;
    int32_t    Estv;
};

/**
 * @brief   全波/基波/半波电参快照（芯片寄存器）
 */
struct msr_input_fullfund
{
    int32_t    UA;                        /*"  offset  0x07        "*/
    int32_t    UB;                        /*"  offset  0x08        "*/
    int32_t    UC;                        /*"  offset  0x09       "*/
    int32_t    USUM;                      /*"  offset  0x0A        "*/
    int32_t    IA;                        /*"  offset  0x0B        "*/
    int32_t    IB;                        /*"  offset  0x0C        "*/
    int32_t    IC;                        /*"  offset  0x0D       "*/
    int32_t    IN;                        /*"  offset  0x0E        "*/
    int32_t    ISUM;                      /*"  offset  0x10        "*/
    int32_t    PA;                        /*"  offset  0x14        "*/
    int32_t    PB;                        /*"  offset  0x15        "*/
    int32_t    PC;                        /*"  offset  0x16        "*/
    int32_t    PT;                        /*"  offset  0x17        "*/
    int32_t    QA;                        /*"  offset  0x18        "*/
    int32_t    QB;                        /*"  offset  0x19        "*/
    int32_t    QC;                        /*"  offset  0x1A        "*/
    int32_t    QT;                        /*"  offset  0x1B        "*/
    int32_t    SA;                        /*"  offset  0x1C        "*/
    int32_t    SB;                        /*"  offset  0x1D        "*/
    int32_t    SC;                        /*"  offset  0x1E        "*/
    int32_t    STA;                       /*"  offset  0x1F        "*/
    int32_t    PfA;                       /*"  offset  0x20        "*/
    int32_t    PfB;                       /*"  offset  0x21        "*/
    int32_t    PfC;                       /*"  offset  0x22        "*/
    int32_t    PfTA;                      /*"  offset  0x23        "*/
    int32_t    EPA;                       /*"  offset  0x30        "*/
    int32_t    EPB;                       /*"  offset  0x31        "*/
    int32_t    EPC;                       /*"  offset  0x32        "*/
    int32_t    EPT;                       /*"  offset  0x33        "*/
    int32_t    EQA;                       /*"  offset  0x3C        "*/
    int32_t    EQB;                       /*"  offset  0x3D        "*/
    int32_t    EQC;                       /*"  offset  0x3E        "*/
    int32_t    EQT;                       /*"  offset  0x3F        "*/
    int32_t    ESA;                       /*"  offset  0x48        "*/
    int32_t    ESB;                       /*"  offset  0x49        "*/
    int32_t    ESC;                       /*"  offset  0x4A        "*/
    int32_t    ESTA;                      /*"  offset  0x4B        "*/
    int32_t    STV;                       /*"  offset  0x4C        "*/
    int32_t    PfTV;                      /*"  offset  0x4D        "*/
    int32_t    ESTV;                      /*"  offset  0x4F        "*/
    int32_t    UFreq;                     /*"  offset  0x57        "*/
    int32_t    YUA;                       /*"  offset  0x50        "*/
    int32_t    YUB;                       /*"  offset  0x51        "*/
    int32_t    YUC;                       /*"  offset  0x52        "*/
    int32_t    YIA;                       /*"  offset  0x53        "*/
    int32_t    YIB;                       /*"  offset  0x54        "*/
    int32_t    YIC;                       /*"  offset  0x55        "*/
    int32_t    YIN;                       /*"  offset  0x56        "*/

    struct msr_input_energy   energy;
};

/**
 * @brief   一拍输入聚合：全波 / 基波 / 半波电参 + 电能增量
 */
struct msr_input
{
    struct msr_input_fullfund full, fund, half;
};

#endif
