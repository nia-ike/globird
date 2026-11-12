/**
 * @file        rn7302_register.h
 * @brief       RN7302 寄存器定义
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302 寄存器定义
 *
 * 说明：C 预处理器不能在宏展开里再写 #define，故用 enum 生成
 *       RN7302_REG_xxx / RN7302_LEN_xxx 整型常量。
 */
#ifndef RN7302_REGISTER_H
#define RN7302_REGISTER_H

/*==============================================================================
 * RN830x / RN730x V3 寄存器宏定义
 * 来源：锐能微RN830x_RN730x V3版用户手册 Rev1.9
 * 3.1 计量参数寄存器列表 + 3.3 配置和状态寄存器列表
 *============================================================================*/

 #define RN7302_INFO(name, addr, len) \
 enum { RN7302_REG_##name = (addr), RN7302_LEN_##name = (len) }

/* -------------------------------------------------------------------------- */
/* Bank0 : 计量参数寄存器 (3.1 表3-1)                                          */
/* 地址映射：0x000 + 偏移                                                      */
/* -------------------------------------------------------------------------- */

/* 全波计量参数寄存器 */
RN7302_INFO(UAWAV,          0x000, 3);   /* A相电压采样数据 */
RN7302_INFO(UBWAV,          0x001, 3);   /* B相电压采样数据 */
RN7302_INFO(UCWAV,          0x002, 3);   /* C相电压采样数据 */
RN7302_INFO(IAWAV,          0x003, 3);   /* A相电流采样数据 */
RN7302_INFO(IBWAV,          0x004, 3);   /* B相电流采样数据 */
RN7302_INFO(ICWAV,          0x005, 3);   /* C相电流采样数据 */
RN7302_INFO(INWAV,          0x006, 3);   /* 零线电流采样数据 */
RN7302_INFO(UA,             0x007, 4);   /* A相电压有效值 */
RN7302_INFO(UB,             0x008, 4);   /* B相电压有效值 */
RN7302_INFO(UC,             0x009, 4);   /* C相电压有效值 */
RN7302_INFO(USUM,           0x00A, 4);   /* 电压矢量和有效值 */
RN7302_INFO(IA,             0x00B, 4);   /* A相电流有效值 */
RN7302_INFO(IB,             0x00C, 4);   /* B相电流有效值 */
RN7302_INFO(IC,             0x00D, 4);   /* C相电流有效值 */
RN7302_INFO(IN,             0x00E, 4);   /* 零线电流有效值 */
RN7302_INFO(ISUM,           0x010, 4);   /* 电流矢量和有效值 */
RN7302_INFO(IA_NVM1,        0x011, 3);   /* NVM1 A相电流有效值 */
RN7302_INFO(IB_NVM1,        0x012, 3);   /* NVM1 B相电流有效值 */
RN7302_INFO(IC_NVM1,        0x013, 3);   /* NVM1 C相电流有效值 */
RN7302_INFO(PA,             0x014, 4);   /* A相有功功率 */
RN7302_INFO(PB,             0x015, 4);   /* B相有功功率 */
RN7302_INFO(PC,             0x016, 4);   /* C相有功功率 */
RN7302_INFO(PT,             0x017, 4);   /* 合相有功功率 */
RN7302_INFO(QA,             0x018, 4);   /* A相无功功率 */
RN7302_INFO(QB,             0x019, 4);   /* B相无功功率 */
RN7302_INFO(QC,             0x01A, 4);   /* C相无功功率 */
RN7302_INFO(QT,             0x01B, 4);   /* 合相无功功率 */
RN7302_INFO(SA,             0x01C, 4);   /* A相视在功率 */
RN7302_INFO(SB,             0x01D, 4);   /* B相视在功率 */
RN7302_INFO(SC,             0x01E, 4);   /* C相视在功率 */
RN7302_INFO(STA,            0x01F, 4);   /* RMS合相视在功率 */
RN7302_INFO(PfA,            0x020, 3);   /* A相功率因数 */
RN7302_INFO(PfB,            0x021, 3);   /* B相功率因数 */
RN7302_INFO(PfC,            0x022, 3);   /* C相功率因数 */
RN7302_INFO(PfTA,           0x023, 3);   /* RMS合相功率因数 */
RN7302_INFO(PAFCnt,         0x024, 3);   /* A相有功快速脉冲计数 */
RN7302_INFO(PBFCnt,         0x025, 3);   /* B相有功快速脉冲计数 */
RN7302_INFO(PCFCnt,         0x026, 3);   /* C相有功快速脉冲计数 */
RN7302_INFO(PTFCnt,         0x027, 3);   /* 合相有功快速脉冲计数 */
RN7302_INFO(QAFCnt,         0x028, 3);   /* A相无功快速脉冲计数 */
RN7302_INFO(QBFCnt,         0x029, 3);   /* B相无功快速脉冲计数 */
RN7302_INFO(QCFCnt,         0x02A, 3);   /* C相无功快速脉冲计数 */
RN7302_INFO(QTFCnt,         0x02B, 3);   /* 合相无功快速脉冲计数 */
RN7302_INFO(SAFCnt,         0x02C, 3);   /* A相视在快速脉冲计数 */
RN7302_INFO(SBFCnt,         0x02D, 3);   /* B相视在快速脉冲计数 */
RN7302_INFO(SCFCnt,         0x02E, 3);   /* C相视在快速脉冲计数 */
RN7302_INFO(STFACnt,        0x02F, 3);   /* RMS合相视在快速脉冲计数 */
RN7302_INFO(EPA,            0x030, 3);   /* A相有功能量寄存器 */
RN7302_INFO(EPB,            0x031, 3);   /* B相有功能量寄存器 */
RN7302_INFO(EPC,            0x032, 3);   /* C相有功能量寄存器 */
RN7302_INFO(EPT,            0x033, 3);   /* 合相有功能量寄存器 */
RN7302_INFO(PosEPA,         0x034, 3);   /* A相正向有功能量寄存器 */
RN7302_INFO(PosEPB,         0x035, 3);   /* B相正向有功能量寄存器 */
RN7302_INFO(PosEPC,         0x036, 3);   /* C相正向有功能量寄存器 */
RN7302_INFO(PosEPT,         0x037, 3);   /* 合相正向有功能量寄存器 */
RN7302_INFO(NegEPA,         0x038, 3);   /* A相反向有功能量寄存器 */
RN7302_INFO(NegEPB,         0x039, 3);   /* B相反向有功能量寄存器 */
RN7302_INFO(NegEPC,         0x03A, 3);   /* C相反向有功能量寄存器 */
RN7302_INFO(NegEPT,         0x03B, 3);   /* 合相反向有功能量寄存器 */
RN7302_INFO(EQA,            0x03C, 3);   /* A相无功能量寄存器 */
RN7302_INFO(EQB,            0x03D, 3);   /* B相无功能量寄存器 */
RN7302_INFO(EQC,            0x03E, 3);   /* C相无功能量寄存器 */
RN7302_INFO(EQT,            0x03F, 3);   /* 合相无功能量寄存器 */
RN7302_INFO(PosEQA,         0x040, 3);   /* A相正向无功能量寄存器 */
RN7302_INFO(PosEQB,         0x041, 3);   /* B相正向无功能量寄存器 */
RN7302_INFO(PosEQC,         0x042, 3);   /* C相正向无功能量寄存器 */
RN7302_INFO(PosEQT,         0x043, 3);   /* 合相正向无功能量寄存器 */
RN7302_INFO(NegEQA,         0x044, 3);   /* A相反向无功能量寄存器 */
RN7302_INFO(NegEQB,         0x045, 3);   /* B相反向无功能量寄存器 */
RN7302_INFO(NegEQC,         0x046, 3);   /* C相反向无功能量寄存器 */
RN7302_INFO(NegEQT,         0x047, 3);   /* 合相反向无功能量寄存器 */
RN7302_INFO(ESA,            0x048, 3);   /* A相视在能量寄存器 */
RN7302_INFO(ESB,            0x049, 3);   /* B相视在能量寄存器 */
RN7302_INFO(ESC,            0x04A, 3);   /* C相视在能量寄存器 */
RN7302_INFO(ESTA,           0x04B, 3);   /* RMS合相视在能量寄存器 */
RN7302_INFO(STV,            0x04C, 4);   /* PQS合相视在功率 */
RN7302_INFO(PfTV,           0x04D, 3);   /* PQS合相功率因数 */
RN7302_INFO(STFVCnt,        0x04E, 3);   /* PQS合相视在快速脉冲计数 */
RN7302_INFO(ESTV,           0x04F, 3);   /* PQS合相视在能量寄存器 */

/* 基波、谐波、半波计量参数寄存器 */
RN7302_INFO(YUA,            0x050, 3);   /* 采样通道UA基波相角寄存器 */
RN7302_INFO(YUB,            0x051, 3);   /* 采样通道UB基波相角寄存器 */
RN7302_INFO(YUC,            0x052, 3);   /* 采样通道UC基波相角寄存器 */
RN7302_INFO(YIA,            0x053, 3);   /* 采样通道IA基波相角寄存器 */
RN7302_INFO(YIB,            0x054, 3);   /* 采样通道IB基波相角寄存器 */
RN7302_INFO(YIC,            0x055, 3);   /* 采样通道IC基波相角寄存器 */
RN7302_INFO(YIN,            0x056, 3);   /* 采样通道IN基波相角寄存器 */
RN7302_INFO(UFreq,          0x057, 3);   /* 电压线频率 */
RN7302_INFO(FUA,            0x058, 4);   /* A相基波电压有效值 */
RN7302_INFO(FUB,            0x059, 4);   /* B相基波电压有效值 */
RN7302_INFO(FUC,            0x05A, 4);   /* C相基波电压有效值 */
RN7302_INFO(FIA,            0x05B, 4);   /* A相基波电流有效值 */
RN7302_INFO(FIB,            0x05C, 4);   /* B相基波电流有效值 */
RN7302_INFO(FIC,            0x05D, 4);   /* C相基波电流有效值 */
RN7302_INFO(FPA,            0x05E, 4);   /* A相基波有功功率 */
RN7302_INFO(FPB,            0x05F, 4);   /* B相基波有功功率 */
RN7302_INFO(FPC,            0x060, 4);   /* C相基波有功功率 */
RN7302_INFO(FPT,            0x061, 4);   /* 合相基波有功功率 */
RN7302_INFO(FQA,            0x062, 4);   /* A相基波无功功率 */
RN7302_INFO(FQB,            0x063, 4);   /* B相基波无功功率 */
RN7302_INFO(FQC,            0x064, 4);   /* C相基波无功功率 */
RN7302_INFO(FQT,            0x065, 4);   /* 合相基波无功功率 */
RN7302_INFO(FSA,            0x066, 4);   /* A相基波视在功率 */
RN7302_INFO(FSB,            0x067, 4);   /* B相基波视在功率 */
RN7302_INFO(FSC,            0x068, 4);   /* C相基波视在功率 */
RN7302_INFO(FSTA,           0x069, 4);   /* RMS合相基波视在功率 */
RN7302_INFO(FPfA,           0x06A, 3);   /* A相基波功率因数 */
RN7302_INFO(FPfB,           0x06B, 3);   /* B相基波功率因数 */
RN7302_INFO(FPfC,           0x06C, 3);   /* C相基波功率因数 */
RN7302_INFO(FPfTA,          0x06D, 3);   /* RMS合相基波功率因数 */
RN7302_INFO(FPAFCnt,        0x06E, 3);   /* A相基波有功快速脉冲计数 */
RN7302_INFO(FPBFCnt,        0x06F, 3);   /* B相基波有功快速脉冲计数 */
RN7302_INFO(FPCFCnt,        0x070, 3);   /* C相基波有功快速脉冲计数 */
RN7302_INFO(FPTFCnt,        0x071, 3);   /* 合相基波有功快速脉冲计数 */
RN7302_INFO(FQAFCnt,        0x072, 3);   /* A相基波无功快速脉冲计数 */
RN7302_INFO(FQBFCnt,        0x073, 3);   /* B相基波无功快速脉冲计数 */
RN7302_INFO(FQCFCnt,        0x074, 3);   /* C相基波无功快速脉冲计数 */
RN7302_INFO(FQTFCnt,        0x075, 3);   /* 合相基波无功快速脉冲计数 */
RN7302_INFO(FSAFCnt,        0x076, 3);   /* A相基波视在快速脉冲计数 */
RN7302_INFO(FSBFCnt,        0x077, 3);   /* B相基波视在快速脉冲计数 */
RN7302_INFO(FSCFCnt,        0x078, 3);   /* C相基波视在快速脉冲计数 */
RN7302_INFO(FSTAFCnt,       0x079, 3);   /* RMS合相基波视在快速脉冲计数 */
RN7302_INFO(FEPA,           0x07A, 3);   /* A相基波有功电能 */
RN7302_INFO(FEPB,           0x07B, 3);   /* B相基波有功电能 */
RN7302_INFO(FEPC,           0x07C, 3);   /* C相基波有功电能 */
RN7302_INFO(FEPT,           0x07D, 3);   /* 合相基波有功电能 */
RN7302_INFO(PosFEPA,        0x07E, 3);   /* A相基波正向有功能量寄存器 */
RN7302_INFO(PosFEPB,        0x07F, 3);   /* B相基波正向有功能量寄存器 */
RN7302_INFO(PosFEPC,        0x080, 3);   /* C相基波正向有功能量寄存器 */
RN7302_INFO(PosFEPT,        0x081, 3);   /* 合相基波正向有功能量寄存器 */
RN7302_INFO(NegFEPA,        0x082, 3);   /* A相基波反向有功能量寄存器 */
RN7302_INFO(NegFEPB,        0x083, 3);   /* B相基波反向有功能量寄存器 */
RN7302_INFO(NegFEPC,        0x084, 3);   /* C相基波反向有功能量寄存器 */
RN7302_INFO(NegFEPT,        0x085, 3);   /* 合相基波反向有功能量寄存器 */
RN7302_INFO(FEQA,           0x086, 3);   /* A相基波无功电能 */
RN7302_INFO(FEQB,           0x087, 3);   /* B相基波无功电能 */
RN7302_INFO(FEQC,           0x088, 3);   /* C相基波无功电能 */
RN7302_INFO(FEQT,           0x089, 3);   /* 合相基波无功电能 */
RN7302_INFO(PosFEQA,        0x08A, 3);   /* A相正向基波无功能量寄存器 */
RN7302_INFO(PosFEQB,        0x08B, 3);   /* B相正向基波无功能量寄存器 */
RN7302_INFO(PosFEQC,        0x08C, 3);   /* C相正向基波无功能量寄存器 */
RN7302_INFO(PosFEQT,        0x08D, 3);   /* 合相正向基波无功能量寄存器 */
RN7302_INFO(NegFEQA,        0x08E, 3);   /* A相反向基波无功能量寄存器 */
RN7302_INFO(NegFEQB,        0x08F, 3);   /* B相反向基波无功能量寄存器 */
RN7302_INFO(NegFEQC,        0x090, 3);   /* C相反向基波无功能量寄存器 */
RN7302_INFO(NegFEQT,        0x091, 3);   /* 合相反向基波无功能量寄存器 */
RN7302_INFO(FESA,           0x092, 3);   /* A相基波视在能量寄存器 */
RN7302_INFO(FESB,           0x093, 3);   /* B相基波视在能量寄存器 */
RN7302_INFO(FESC,           0x094, 3);   /* C相基波视在能量寄存器 */
RN7302_INFO(FESTA,          0x095, 3);   /* RMS合相基波视在能量寄存器 */
RN7302_INFO(HUA,            0x096, 4);   /* A相谐波电压有效值 */
RN7302_INFO(HUB,            0x097, 4);   /* B相谐波电压有效值 */
RN7302_INFO(HUC,            0x098, 4);   /* C相谐波电压有效值 */
RN7302_INFO(HIA,            0x099, 4);   /* A相谐波电流有效值 */
RN7302_INFO(HIB,            0x09A, 4);   /* B相谐波电流有效值 */
RN7302_INFO(HIC,            0x09B, 4);   /* C相谐波电流有效值 */
RN7302_INFO(FSTV,           0x09C, 4);   /* PQS合相基波视在功率 */
RN7302_INFO(FPfTV,          0x09D, 3);   /* PQS合相基波功率因数 */
RN7302_INFO(FSTVFCnt,       0x09E, 3);   /* PQS合相基波视在快速脉冲计数 */
RN7302_INFO(FESTV,          0x09F, 3);   /* PQS合相基波视在能量寄存器 */
RN7302_INFO(UA_HW,          0x0A0, 4);   /* A相电压半波有效值 */
RN7302_INFO(UB_HW,          0x0A1, 4);   /* B相电压半波有效值 */
RN7302_INFO(UC_HW,          0x0A2, 4);   /* C相电压半波有效值 */
RN7302_INFO(IA_HW,          0x0A3, 4);   /* A相电流半波有效值 */
RN7302_INFO(IB_HW,          0x0A4, 4);   /* B相电流半波有效值 */
RN7302_INFO(IC_HW,          0x0A5, 4);   /* C相电流半波有效值 */
RN7302_INFO(UAPEAK_HW,      0x0A6, 2);   /* A相电压半波峰值 */
RN7302_INFO(UBPEAK_HW,      0x0A7, 2);   /* B相电压半波峰值 */
RN7302_INFO(UCPEAK_HW,      0x0A8, 2);   /* C相电压半波峰值 */
RN7302_INFO(IN_HW,          0x0A9, 4);   /* 零线电流半波有效值 */
RN7302_INFO(FUA_WAVE,       0x0AA, 3);   /* A相电压基波瞬时采样波形 */
RN7302_INFO(FUB_WAVE,       0x0AB, 3);   /* B相电压基波瞬时采样波形 */
RN7302_INFO(FUC_WAVE,       0x0AC, 3);   /* C相电压基波瞬时采样波形 */
RN7302_INFO(FIA_WAVE,       0x0AD, 3);   /* A相电流基波瞬时采样波形 */
RN7302_INFO(FIB_WAVE,       0x0AE, 3);   /* B相电流基波瞬时采样波形 */
RN7302_INFO(FIC_WAVE,       0x0AF, 3);   /* C相电流基波瞬时采样波形 */
RN7302_INFO(FIN_WAVE,       0x0B0, 3);   /* 零线电流基波瞬时采样波形 */
RN7302_INFO(SPL_PA,         0x0B1, 4);   /* A相瞬时有功功率 (半波有功功率) */
RN7302_INFO(SPL_PB,         0x0B2, 4);   /* B相瞬时有功功率 (半波有功功率) */
RN7302_INFO(SPL_PC,         0x0B3, 4);   /* C相瞬时有功功率 (半波有功功率) */
RN7302_INFO(SPL_QA,         0x0B4, 4);   /* A相瞬时无功功率 (半波无功功率) */
RN7302_INFO(SPL_QB,         0x0B5, 4);   /* B相瞬时无功功率 (半波无功功率) */
RN7302_INFO(SPL_QC,         0x0B6, 4);   /* C相瞬时无功功率 (半波无功功率) */
RN7302_INFO(SPL_FPA,        0x0B7, 4);   /* A相瞬时基波有功功率 */
RN7302_INFO(SPL_FPB,        0x0B8, 4);   /* B相瞬时基波有功功率 */
RN7302_INFO(SPL_FPC,        0x0B9, 4);   /* C相瞬时基波有功功率 */
RN7302_INFO(SPL_FQA,        0x0BA, 4);   /* A相瞬时基波无功功率 */
RN7302_INFO(SPL_FQB,        0x0BB, 4);   /* B相瞬时基波无功功率 */
RN7302_INFO(SPL_FQC,        0x0BC, 4);   /* C相瞬时基波无功功率 */
RN7302_INFO(RMS_LA1,        0x0BD, 3);   /* 全失压测量IA有效值(单次更新) */
RN7302_INFO(RMS_LB1,        0x0BE, 3);   /* 全失压测量IB有效值(单次更新) */
RN7302_INFO(RMS_LC1,        0x0BF, 3);   /* 全失压测量IC有效值(单次更新) */
RN7302_INFO(FLK_UA,         0x0C0, 4);   /* UA通道电压瞬时值 */
RN7302_INFO(FLK_UB,         0x0C1, 4);   /* UB通道电压瞬时值 */
RN7302_INFO(FLK_UC,         0x0C2, 4);   /* UC通道电压瞬时值 */
RN7302_INFO(SAG_UAPEAK,     0x0C3, 2);   /* UA通道SAG峰值最小值寄存器 */
RN7302_INFO(SAG_UBPEAK,     0x0C4, 2);   /* UB通道SAG峰值最小值寄存器 */
RN7302_INFO(SAG_UCPEAK,     0x0C5, 2);   /* UC通道SAG峰值最小值寄存器 */
RN7302_INFO(UA_HW2,         0x0C6, 4);   /* A相电压半波有效值2 */
RN7302_INFO(UB_HW2,         0x0C7, 4);   /* B相电压半波有效值2 */
RN7302_INFO(UC_HW2,         0x0C8, 4);   /* C相电压半波有效值2 */
RN7302_INFO(IA_HW2,         0x0C9, 4);   /* A相电流半波有效值2 */
RN7302_INFO(IB_HW2,         0x0CA, 4);   /* B相电流半波有效值2 */
RN7302_INFO(IC_HW2,         0x0CB, 4);   /* C相电流半波有效值2 */
RN7302_INFO(IN_HW2,         0x0CC, 4);   /* 零线电流半波有效值2 */
RN7302_INFO(FLK_RSLTA,      0x0CD, 4);   /* UA通道闪变寄存器 */
RN7302_INFO(FLK_RSLTB,      0x0CE, 4);   /* UB通道闪变寄存器 */
RN7302_INFO(FLK_RSLTC,      0x0CF, 4);   /* UC通道闪变寄存器 */
RN7302_INFO(PTFCnt2,        0x0F9, 3);   /* 合相有功快速脉冲计数寄存器2 */
RN7302_INFO(QTFCnt2,        0x0FA, 3);   /* 合相无功快速脉冲计数寄存器2 */
RN7302_INFO(STFCnt2,        0x0FB, 3);   /* 合相视在快速脉冲计数寄存器2 */
RN7302_INFO(FPTFCnt2,       0x0FC, 3);   /* 合相基波有功快速脉冲计数寄存器2 */
RN7302_INFO(FQTFCnt2,       0x0FD, 3);   /* 合相基波无功快速脉冲计数寄存器2 */
RN7302_INFO(FSTFCnt2,       0x0FE, 3);   /* 合相基波视在快速脉冲计数寄存器2 */

/* -------------------------------------------------------------------------- */
/* Bank6 : 计量参数寄存器2 (3.1 表3-2)                                         */
/* 地址映射：0x600 + 偏移                                                      */
/* -------------------------------------------------------------------------- */
RN7302_INFO(HW_PA,          0x600, 4);   /* A相半波有功功率 */
RN7302_INFO(HW_PB,          0x601, 4);   /* B相半波有功功率 */
RN7302_INFO(HW_PC,          0x602, 4);   /* C相半波有功功率 */
RN7302_INFO(HW_QA,          0x603, 4);   /* A相半波无功功率 */
RN7302_INFO(HW_QB,          0x604, 4);   /* B相半波无功功率 */
RN7302_INFO(HW_QC,          0x605, 4);   /* C相半波无功功率 */
RN7302_INFO(HW_FPA,         0x606, 4);   /* A相基波半波有功功率 */
RN7302_INFO(HW_FPB,         0x607, 4);   /* B相基波半波有功功率 */
RN7302_INFO(HW_FPC,         0x608, 4);   /* C相基波半波有功功率 */
RN7302_INFO(HW_FQA,         0x609, 4);   /* A相基波半波无功功率 */
RN7302_INFO(HW_FQB,         0x60A, 4);   /* B相基波半波无功功率 */
RN7302_INFO(HW_FQC,         0x60B, 4);   /* C相基波半波无功功率 */
RN7302_INFO(SPL_FIA,        0x60C, 4);   /* A相瞬时基波电流有效值 */
RN7302_INFO(SPL_FIB,        0x60D, 4);   /* B相瞬时基波电流有效值 */
RN7302_INFO(SPL_FIC,        0x60E, 4);   /* C相瞬时基波电流有效值 */
RN7302_INFO(SPL_FUA,        0x60F, 4);   /* A相瞬时基波电压有效值 */
RN7302_INFO(SPL_FUB,        0x610, 4);   /* B相瞬时基波电压有效值 */
RN7302_INFO(SPL_FUC,        0x611, 4);   /* C相瞬时基波电压有效值 */

/* -------------------------------------------------------------------------- */
/* Bank1 : 配置和状态寄存器 (3.3 表3-2)                                         */
/* 地址映射：0x100 + 偏移                                                      */
/* -------------------------------------------------------------------------- */

/* EMM 校表寄存器 I */
RN7302_INFO(HFConst1,       0x100, 2);   /* 高频脉冲计数寄存器1 */
RN7302_INFO(HFConst2,       0x101, 2);   /* 高频脉冲计数寄存器2 */
RN7302_INFO(IStart_PS,      0x102, 2);   /* 有功视在启动电流阈值寄存器 */
RN7302_INFO(IStart_Q,       0x103, 2);   /* 无功启动电流阈值寄存器 */
RN7302_INFO(LostVoltT,      0x104, 2);   /* 失压阈值寄存器 */
RN7302_INFO(ZXOT,           0x105, 2);   /* 过零阈值寄存器 */
RN7302_INFO(PRTH1L,         0x106, 2);   /* 相位分段校正电流阈值1下限 */
RN7302_INFO(PRTH1H,         0x107, 2);   /* 相位分段校正电流阈值1上限 */
RN7302_INFO(PRTH2L,         0x108, 2);   /* 相位分段校正电流阈值2下限 */
RN7302_INFO(PRTH2H,         0x109, 2);   /* 相位分段校正电流阈值2上限 */
RN7302_INFO(IRegion3L,      0x10A, 2);   /* 电流阈值3下限 */
RN7302_INFO(IRegion3H,      0x10B, 2);   /* 电流阈值3上限 */
RN7302_INFO(PHSUA,          0x10C, 1);   /* 采样通道UA相位校正寄存器 */
RN7302_INFO(PHSUB,          0x10D, 1);   /* 采样通道UB相位校正寄存器 */
RN7302_INFO(PHSUC,          0x10E, 1);   /* 采样通道UC相位校正寄存器 */
RN7302_INFO(ExPHSUA,        0x10C, 4);   /* 扩展采样通道UA相位校正寄存器 */
RN7302_INFO(ExPHSUB,        0x10D, 4);   /* 扩展采样通道UB相位校正寄存器 */
RN7302_INFO(ExPHSUC,        0x10E, 4);   /* 扩展采样通道UC相位校正寄存器 */
RN7302_INFO(PHSIA,          0x10F, 3);   /* 采样通道IA分段相位校正寄存器 */
RN7302_INFO(PHSIB,          0x110, 3);   /* 采样通道IB分段相位校正寄存器 */
RN7302_INFO(PHSIC,          0x111, 3);   /* 采样通道IC分段相位校正寄存器 */
RN7302_INFO(PHSIN,          0x112, 1);   /* 采样通道IN相位校正 */
RN7302_INFO(ExPHSIA,        0x10F, 4);   /* 扩展采样通道IA分段相位校正寄存器 */
RN7302_INFO(ExPHSIB,        0x110, 4);   /* 扩展采样通道IB分段相位校正寄存器 */
RN7302_INFO(ExPHSIC,        0x111, 4);   /* 扩展采样通道IC分段相位校正寄存器 */
RN7302_INFO(GSUA,           0x113, 2);   /* 采样通道UA通道增益 */
RN7302_INFO(GSUB,           0x114, 2);   /* 采样通道UB通道增益 */
RN7302_INFO(GSUC,           0x115, 2);   /* 采样通道UC通道增益 */
RN7302_INFO(GSIA,           0x116, 2);   /* 采样通道IA通道增益 */
RN7302_INFO(GSIB,           0x117, 2);   /* 采样通道IB通道增益 */
RN7302_INFO(GSIC,           0x118, 2);   /* 采样通道IC通道增益 */
RN7302_INFO(GSIN,           0x119, 2);   /* 采样通道IN通道增益 */
RN7302_INFO(DCOS_UA,        0x11A, 2);   /* 采样通道UA直流Offset校正 */
RN7302_INFO(DCOS_UB,        0x11B, 2);   /* 采样通道UB直流Offset校正 */
RN7302_INFO(DCOS_UC,        0x11C, 2);   /* 采样通道UC直流Offset校正 */
RN7302_INFO(DCOS_IA,        0x11D, 2);   /* 采样通道IA直流Offset校正 */
RN7302_INFO(DCOS_IB,        0x11E, 2);   /* 采样通道IB直流Offset校正 */
RN7302_INFO(DCOS_IC,        0x11F, 2);   /* 采样通道IC直流Offset校正 */
RN7302_INFO(DCOS_IN,        0x120, 2);   /* 采样通道IN直流Offset校正 */
RN7302_INFO(UA_OS,          0x121, 2);   /* A相电压有效值Offset */
RN7302_INFO(UB_OS,          0x122, 2);   /* B相电压有效值Offset */
RN7302_INFO(UC_OS,          0x123, 2);   /* C相电压有效值Offset */
RN7302_INFO(IA_OS,          0x124, 2);   /* A相电流有效值Offset */
RN7302_INFO(IB_OS,          0x125, 2);   /* B相电流有效值Offset */
RN7302_INFO(IC_OS,          0x126, 2);   /* C相电流有效值Offset */
RN7302_INFO(IN_OS,          0x127, 2);   /* 零线电流有效值Offset */
RN7302_INFO(GPA,            0x128, 2);   /* A相有功功率增益 */
RN7302_INFO(GPB,            0x129, 2);   /* B相有功功率增益 */
RN7302_INFO(GPC,            0x12A, 2);   /* C相有功功率增益 */
RN7302_INFO(GQA,            0x12B, 2);   /* A相无功功率增益 */
RN7302_INFO(GQB,            0x12C, 2);   /* B相无功功率增益 */
RN7302_INFO(GQC,            0x12D, 2);   /* C相无功功率增益 */
RN7302_INFO(GSA,            0x12E, 2);   /* A相视在功率增益 */
RN7302_INFO(GSB,            0x12F, 2);   /* B相视在功率增益 */
RN7302_INFO(GSC,            0x130, 2);   /* C相视在功率增益 */
RN7302_INFO(PA_PHSL,        0x131, 2);   /* A相有功分段相位校正寄存器-低段 */
RN7302_INFO(PB_PHSL,        0x132, 2);   /* B相有功分段相位校正寄存器-低段 */
RN7302_INFO(PC_PHSL,        0x133, 2);   /* C相有功分段相位校正寄存器-低段 */
RN7302_INFO(QA_PHSL,        0x134, 2);   /* A相无功分段相位校正寄存器-低段 */
RN7302_INFO(QB_PHSL,        0x135, 2);   /* B相无功分段相位校正寄存器-低段 */
RN7302_INFO(QC_PHSL,        0x136, 2);   /* C相无功分段相位校正寄存器-低段 */
RN7302_INFO(PA_OS,          0x137, 2);   /* A相有功功率Offset */
RN7302_INFO(PB_OS,          0x138, 2);   /* B相有功功率Offset */
RN7302_INFO(PC_OS,          0x139, 2);   /* C相有功功率Offset */
RN7302_INFO(QA_OS,          0x13A, 2);   /* A相无功功率Offset */
RN7302_INFO(QB_OS,          0x13B, 2);   /* B相无功功率Offset */
RN7302_INFO(QC_OS,          0x13C, 2);   /* C相无功功率Offset */
RN7302_INFO(FUA_OS,         0x13D, 2);   /* A相基波电压有效值Offset */
RN7302_INFO(FUB_OS,         0x13E, 2);   /* B相基波电压有效值Offset */
RN7302_INFO(FUC_OS,         0x13F, 2);   /* C相基波电压有效值Offset */
RN7302_INFO(FIA_OS,         0x140, 2);   /* A相基波电流有效值Offset */
RN7302_INFO(FIB_OS,         0x141, 2);   /* B相基波电流有效值Offset */
RN7302_INFO(FIC_OS,         0x142, 2);   /* C相基波电流有效值Offset */
RN7302_INFO(GFPA,           0x143, 2);   /* A相基波有功功率增益 */
RN7302_INFO(GFPB,           0x144, 2);   /* B相基波有功功率增益 */
RN7302_INFO(GFPC,           0x145, 2);   /* C相基波有功功率增益 */
RN7302_INFO(GFQA,           0x146, 2);   /* A相基波无功功率增益 */
RN7302_INFO(GFQB,           0x147, 2);   /* B相基波无功功率增益 */
RN7302_INFO(GFQC,           0x148, 2);   /* C相基波无功功率增益 */
RN7302_INFO(GFSA,           0x149, 2);   /* A相基波视在功率增益 */
RN7302_INFO(GFSB,           0x14A, 2);   /* B相基波视在功率增益 */
RN7302_INFO(GFSC,           0x14B, 2);   /* C相基波视在功率增益 */
RN7302_INFO(FPA_PHS,        0x14C, 2);   /* A相基波有功相位校正寄存器 */
RN7302_INFO(FPB_PHS,        0x14D, 2);   /* B相基波有功相位校正寄存器 */
RN7302_INFO(FPC_PHS,        0x14E, 2);   /* C相基波有功相位校正寄存器 */
RN7302_INFO(FQA_PHS,        0x14F, 2);   /* A相基波无功相位校正寄存器 */
RN7302_INFO(FQB_PHS,        0x150, 2);   /* B相基波无功相位校正寄存器 */
RN7302_INFO(FQC_PHS,        0x151, 2);   /* C相基波无功相位校正寄存器 */
RN7302_INFO(FPA_OS,         0x152, 2);   /* A相基波有功功率Offset */
RN7302_INFO(FPB_OS,         0x153, 2);   /* B相基波有功功率Offset */
RN7302_INFO(FPC_OS,         0x154, 2);   /* C相基波有功功率Offset */
RN7302_INFO(FQA_OS,         0x155, 2);   /* A相基波无功功率Offset */
RN7302_INFO(FQB_OS,         0x156, 2);   /* B相基波无功功率Offset */
RN7302_INFO(FQC_OS,         0x157, 2);   /* C相基波无功功率Offset */
RN7302_INFO(SAGCFG,         0x158, 3);   /* 电压暂降阈值配置 */
RN7302_INFO(OVLVL,          0x159, 2);   /* 过压阈值配置 */
RN7302_INFO(OILVL,          0x15A, 2);   /* 过流阈值配置 */
RN7302_INFO(ISUM_OS,        0x15B, 2);   /* 电流矢量和OFFSET校正寄存器 */
RN7302_INFO(HFConst0,       0x15C, 2);   /* 高频脉冲计数寄存器0 */
RN7302_INFO(UCHSEL,         0x15D, 1);   /* 电压通道选择寄存器 */
RN7302_INFO(TTEEC_EN,       0x15E, 1);   /* 三相三线错误接线计量更正使能寄存器 */

/* EMM 配置和状态寄存器 */
RN7302_INFO(CFCFG,          0x160, 3);   /* CF引脚配置寄存器 */
RN7302_INFO(EMUCFG,         0x161, 3);   /* EMU配置寄存器 */
RN7302_INFO(EMUCON,         0x162, 3);   /* EMU控制寄存器 */
RN7302_INFO(WSAVECON,       0x163, 1);   /* 采样数据写缓存控制寄存器 */
RN7302_INFO(EMMIE,          0x164, 2);   /* EMM中断允许寄存器 */
RN7302_INFO(EMMIF,          0x165, 2);   /* EMM中断标志和状态寄存器 */
RN7302_INFO(PQSign,         0x166, 2);   /* 有功无功功率方向寄存器 */
RN7302_INFO(Noload,         0x167, 2);   /* 潜动启动状态寄存器 */
RN7302_INFO(IRegionS,       0x168, 1);   /* 电流分区状态寄存器 */
RN7302_INFO(PHASES,         0x169, 2);   /* 相电压电流状态寄存器 */
RN7302_INFO(CheckSum1,      0x16A, 3);   /* EMM校表和配置寄存器校验和 */
RN7302_INFO(EMMIE2,         0x16B, 4);   /* EMM中断允许寄存器2 */
RN7302_INFO(EMMIF2,         0x16C, 4);   /* EMM中断标志和状态寄存器2 */
RN7302_INFO(EMUCFG3,        0x16D, 4);   /* EMU配置寄存器3 */
RN7302_INFO(EMUCFG4,        0x16E, 4);   /* EMU配置寄存器4 */
RN7302_INFO(SPECFG,         0x16F, 4);   /* EMU特殊功能配置寄存器 */

/* NVM1 / NVM2 / 系统配置 */
RN7302_INFO(NVM1CFG,        0x170, 1);   /* NVM1配置寄存器 */
RN7302_INFO(NVM1IF,         0x171, 1);   /* NVM1状态寄存器 */
RN7302_INFO(NVM2CFG,        0x172, 2);   /* NVM2配置寄存器 */
RN7302_INFO(NVM2CMPA,       0x173, 2);   /* NVM2 IA比较器控制寄存器 */
RN7302_INFO(NVM2CMPB,       0x174, 2);   /* NVM2 IB比较器控制寄存器 */
RN7302_INFO(NVM2CMPC,       0x175, 2);   /* NVM2 IC比较器控制寄存器 */
RN7302_INFO(NVM2IF,         0x176, 1);   /* 全失压状态寄存器 */
RN7302_INFO(SYSCFG,         0x178, 4);   /* 系统配置寄存器 */
RN7302_INFO(WREN2,          0x17F, 4);   /* 写使能寄存器2 */
RN7302_INFO(WREN,           0x180, 1);   /* 写使能寄存器 */
RN7302_INFO(WMSW,           0x181, 1);   /* 工作模式切换寄存器 */
RN7302_INFO(SOFTRST,        0x182, 1);   /* 软件复位寄存器 */
RN7302_INFO(ADCCFG,         0x183, 2);   /* ADC配置寄存器 */
RN7302_INFO(TFEEC_EN,       0x184, 1);   /* 三相四线错误接线计量更正使能寄存器 */
RN7302_INFO(ICHSEL,         0x185, 1);   /* 电流通道选择寄存器 */
RN7302_INFO(MODSEL,         0x186, 1);   /* 三相四线/三相三线模式选择寄存器 */
RN7302_INFO(SYSSR,          0x18A, 2);   /* 系统状态寄存器 */
RN7302_INFO(CheckSum2,      0x18B, 2);   /* NVM1、NVM2、系统配置寄存器校验和 */
RN7302_INFO(WData,          0x18D, 4);   /* 上一次SPI写入的数据 (3/4字节) */
RN7302_INFO(LRBufAddr,      0x18E, 2);   /* 最后一次读波形缓存的地址 */
RN7302_INFO(DeviceID,       0x18F, 3);   /* Device ID */

/* EMM 校表寄存器 II */
RN7302_INFO(ZXOTU,          0x190, 2);   /* 电压夹角及测频阈值寄存器 */
RN7302_INFO(AUTODC_EN,      0x191, 2);   /* 直流OFFSET自动校正使能寄存器 */
RN7302_INFO(ZXOTCFG,        0x192, 2);   /* 过零计算配置及标志寄存器 */
RN7302_INFO(DMA_WAVECON,    0x193, 4);   /* 同步采样通道配置寄存器 */
RN7302_INFO(IN_WaveEN,      0x194, 2);   /* IN波形缓存使能寄存器 */
RN7302_INFO(OI2_LVL,        0x198, 2);   /* 过流幅值阈值配置寄存器 */
RN7302_INFO(OI2_CNTT,       0x199, 1);   /* 过流时间阈值配置寄存器 */
RN7302_INFO(EMUCFG2,        0x19A, 4);   /* EMU配置选择寄存器2 */
RN7302_INFO(CFCFG1,         0x19B, 4);   /* CF引脚配置寄存器1 */
RN7302_INFO(OV2_LVL,        0x19C, 2);   /* 过压幅值阈值配置寄存器 */
RN7302_INFO(OV2_CNTTH,      0x19D, 2);   /* 过压时间阈值配置寄存器 */

/* 分段相位校正 / 自定义功率 (A2~AD / B0~BB) */
RN7302_INFO(PA_PHSM,        0x1A2, 2);   /* A相有功分段相位校正-中段 */
RN7302_INFO(PA_PHSH,        0x1A3, 2);   /* A相有功分段相位校正-高段 */
RN7302_INFO(PB_PHSM,        0x1A4, 2);   /* B相有功分段相位校正-中段 */
RN7302_INFO(PB_PHSH,        0x1A5, 2);   /* B相有功分段相位校正-高段 */
RN7302_INFO(PC_PHSM,        0x1A6, 2);   /* C相有功分段相位校正-中段 */
RN7302_INFO(PC_PHSH,        0x1A7, 2);   /* C相有功分段相位校正-高段 */
RN7302_INFO(QA_PHSM,        0x1A8, 2);   /* A相无功分段相位校正-中段 */
RN7302_INFO(QA_PHSH,        0x1A9, 2);   /* A相无功分段相位校正-高段 */
RN7302_INFO(QB_PHSM,        0x1AA, 2);   /* B相无功分段相位校正-中段 */
RN7302_INFO(QB_PHSH,        0x1AB, 2);   /* B相无功分段相位校正-高段 */
RN7302_INFO(QC_PHSM,        0x1AC, 2);   /* C相无功分段相位校正-中段 */
RN7302_INFO(QC_PHSH,        0x1AD, 2);   /* C相无功分段相位校正-高段 */
RN7302_INFO(CHECKSUM3,      0x1AE, 4);   /* 校验和寄存器3 */

/* B0~BB 分段相位 / 自定义功率 (3字节) */
RN7302_INFO(PA_PHSM_D2FP00, 0x1B0, 3);   /* A相有功中段 / 自定义功率0 */
RN7302_INFO(PA_PHSH_D2FP01, 0x1B1, 3);   /* A相有功高段 / 自定义功率1 */
RN7302_INFO(PB_PHSM_D2FP02, 0x1B2, 3);   /* B相有功中段 / 自定义功率2 */
RN7302_INFO(PB_PHSH_D2FP03, 0x1B3, 3);   /* B相有功高段 / 自定义功率3 */
RN7302_INFO(PC_PHSM_D2FP04, 0x1B4, 3);   /* C相有功中段 / 自定义功率4 */
RN7302_INFO(PC_PHSH_D2FP05, 0x1B5, 3);   /* C相有功高段 / 自定义功率5 */
RN7302_INFO(QA_PHSM_D2FP06, 0x1B6, 3);   /* A相无功中段 / 自定义功率6 */
RN7302_INFO(QA_PHSH_D2FP07, 0x1B7, 3);   /* A相无功高段 / 自定义功率7 */
RN7302_INFO(QB_PHSM_D2FP08, 0x1B8, 3);   /* B相无功中段 / 自定义功率8 */
RN7302_INFO(QB_PHSH_D2FP09, 0x1B9, 3);   /* B相无功高段 / 自定义功率9 */
RN7302_INFO(QC_PHSM_D2FP10, 0x1BA, 3);   /* C相无功中段 / 自定义功率10 */
RN7302_INFO(QC_PHSH_D2FP11, 0x1BB, 3);   /* C相无功高段 / 自定义功率11 */

/* 全失压相关 */
RN7302_INFO(LS_DCOS_IA,     0x1CB, 2);   /* 全失压IA测量时直流offset校正 */
RN7302_INFO(LS_DCOS_IB,     0x1CC, 2);   /* 全失压IB测量时直流offset校正 */
RN7302_INFO(LS_DCOS_IC,     0x1CD, 2);   /* 全失压IC测量时直流offset校正 */
RN7302_INFO(LS_THOA,        0x1CE, 3);   /* 全失压IA测量阈值设置寄存器 */
RN7302_INFO(LS_THOB,        0x1CF, 3);   /* 全失压IB测量阈值设置寄存器 */
RN7302_INFO(LS_THOC,        0x1D0, 3);   /* 全失压IC测量阈值设置寄存器 */

/* 同步采样通道配置 */
RN7302_INFO(DMA_WAVECON2,   0x1D1, 4);   /* 同步采样通道配置寄存器2 */
RN7302_INFO(PHS_UA_WAVE,    0x1D2, 2);   /* 同步采样UA通道相位校正寄存器 */
RN7302_INFO(PHS_UB_WAVE,    0x1D3, 2);   /* 同步采样UB通道相位校正寄存器 */
RN7302_INFO(PHS_UC_WAVE,    0x1D4, 2);   /* 同步采样UC通道相位校正寄存器 */
RN7302_INFO(PHS_IA_WAVE,    0x1D5, 2);   /* 同步采样IA通道相位校正寄存器 */
RN7302_INFO(PHS_IB_WAVE,    0x1D6, 2);   /* 同步采样IB通道相位校正寄存器 */
RN7302_INFO(PHS_IC_WAVE,    0x1D7, 2);   /* 同步采样IC通道相位校正寄存器 */
RN7302_INFO(PHS_IN_WAVE,    0x1D8, 2);   /* 同步采样IN通道相位校正寄存器 */
RN7302_INFO(GSUA_WAVE,      0x1D9, 2);   /* 同步采样UA通道增益校正寄存器 */
RN7302_INFO(GSUB_WAVE,      0x1DA, 2);   /* 同步采样UB通道增益校正寄存器 */
RN7302_INFO(GSUC_WAVE,      0x1DB, 2);   /* 同步采样UC通道增益校正寄存器 */
RN7302_INFO(GSIA_WAVE,      0x1DC, 2);   /* 同步采样IA通道增益校正寄存器 */
RN7302_INFO(GSIB_WAVE,      0x1DD, 2);   /* 同步采样IB通道增益校正寄存器 */
RN7302_INFO(GSIC_WAVE,      0x1DE, 2);   /* 同步采样IC通道增益校正寄存器 */
RN7302_INFO(GSIN_WAVE,      0x1DF, 2);   /* 同步采样IN通道增益校正寄存器 */

/* 其它配置 */
RN7302_INFO(DMA_WAVECON3,   0x1E5, 4);   /* 波形采样通道配置寄存器3 */
RN7302_INFO(HW2_CFG,        0x1E6, 4);   /* 半波有效值2配置寄存器 */
RN7302_INFO(LVDCFG,         0x1E7, 4);   /* 电压暂降检测阈值配置寄存器 */
RN7302_INFO(HW_PNum,        0x1E8, 4);   /* 半波有功功率计算点数配置寄存器 */
RN7302_INFO(HW_QNum,        0x1E9, 4);   /* 半波无功功率计算点数配置寄存器 */
RN7302_INFO(HW_MODCFG,      0x1EA, 4);   /* 半波功率模式配置寄存器 */
RN7302_INFO(ROS_DCATC,      0x1EB, 4);   /* 罗氏线圈积分直流衰减系数寄存器 */
RN7302_INFO(ROS_Tran_K,     0x1EC, 4);   /* 罗氏线圈积分转换系数寄存器 */
RN7302_INFO(ROS_CFG,        0x1ED, 4);   /* 罗氏线圈配置寄存器 */
RN7302_INFO(CHECKSUM4,      0x1EF, 4);   /* 配置寄存器校验和寄存器4 */
RN7302_INFO(DeviceID2,      0x1FF, 3);   /* DeviceID2 */

#endif
