/**
 * @file        rn7302.c
 * @brief       RN7302硬件初始化
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   RN7302硬件初始化
 */
#include <stdio.h>
#include "rn7302.h"
#include "port_rn7302.h"
#include "rn7302_register.h"
#include "rn7302_register_config.h"
#include "rn7302_phasecheck.h"

/**
 * @brief       RN7302 CFCFG 初始化
 * @param[in]    cfg    配置
 * @return      void
 */
static void rn7302_cfcfg_init(struct msr_config *cfg)
{
    int32_t tmp = 0;
    int is_activepower = (cfg->user.pulse_mode == 0) ? 1 : 0;
    int is_fullwave    = (rn7302_phasecheck_is_happened(cfg) == PHASECHECK_NOT_HAPPENED) ? 1 : 0;

    if (is_activepower) {
        /* 有功脉冲 */
        tmp = is_fullwave ? RN7302_CFCFG_CF1CFG_P : RN7302_CFCFG_CF1CFG_FP;
    }
    else {
        /* 无功脉冲 */
        tmp = is_fullwave ? RN7302_CFCFG_CF1CFG_Q : RN7302_CFCFG_CF1CFG_FQ;
    }
    MeterChipWriteReg(RN7302_REG_CFCFG, RN7302_LEN_CFCFG, tmp);
}

/**
 * @brief       RN7302电压、电流相位初始化
 * @param[in]    cfg    配置
 * @return      void
 */
static void rn7302_phs_init(struct msr_config *cfg)
{
    if (cfg->sys.chip.PA_PHSL == 0) {
        /* 写使能2 */
        MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, RN7302_WREN2_CMD_WREN2);

        /* 配置电压通道相位 */
        MeterChipWriteReg(RN7302_REG_ExPHSUA, RN7302_LEN_ExPHSUA, 0xA7 << 24);
        MeterChipWriteReg(RN7302_REG_ExPHSUB, RN7302_LEN_ExPHSUB, 0xA7 << 24);
        MeterChipWriteReg(RN7302_REG_ExPHSUC, RN7302_LEN_ExPHSUC, 0xA7 << 24);
        MeterChipWriteReg(RN7302_REG_ExPHSUA, RN7302_LEN_ExPHSUA, ((0xA7 << 24) | 0x200) + cfg->sys.chip.PHSUA);
        MeterChipWriteReg(RN7302_REG_ExPHSUB, RN7302_LEN_ExPHSUB, ((0xA7 << 24) | 0x200) + cfg->sys.chip.PHSUB);
        MeterChipWriteReg(RN7302_REG_ExPHSUC, RN7302_LEN_ExPHSUC, ((0xA7 << 24) | 0x200) + cfg->sys.chip.PHSUC);

        /* 配置电流通道相位 */
        int32_t PHSIX[3] = {0};
        PHSIX[0] = ((cfg->sys.chip.PHSIA & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR1) | (((cfg->sys.chip.PHSIA >> 10 & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR2)<< 10) | ((((cfg->sys.chip.PHSIA >> 20) & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR3) << 20);
        PHSIX[1] = ((cfg->sys.chip.PHSIB & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR1) | (((cfg->sys.chip.PHSIB >> 10 & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR2)<< 10) | ((((cfg->sys.chip.PHSIB >> 20) & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR3) << 20);
        PHSIX[2] = ((cfg->sys.chip.PHSIC & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR1) | (((cfg->sys.chip.PHSIC >> 10 & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR2)<< 10) | ((((cfg->sys.chip.PHSIC >> 20) & 0x3FF) + (int16_t)cfg->sys.chip.PHsxR3) << 20);
        MeterChipWriteReg(RN7302_REG_ExPHSIA, RN7302_LEN_ExPHSIA, PHSIX[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[0])]);
        MeterChipWriteReg(RN7302_REG_ExPHSIB, RN7302_LEN_ExPHSIB, PHSIX[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[1])]);
        MeterChipWriteReg(RN7302_REG_ExPHSIC, RN7302_LEN_ExPHSIC, PHSIX[rn7302_phasecheck_trans2idx(cfg->user.phase_adjust[2])]);

        if (cfg->sys.adjust.rated_current == 0) {
            printf("Warning: Adjust_I.value is 0, using default value 1 to avoid division by zero\n");
            cfg->sys.adjust.rated_current = 1;
        }

        /* 配置相位分段校正电流阈值 */
        int32_t tmp_val = 0;
        tmp_val = (cfg->sys.chip.PRTH1L * cfg->sys.adjust.reg_ia / cfg->sys.adjust.rated_current / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH1L, RN7302_LEN_PRTH1L, tmp_val);
        tmp_val = (cfg->sys.chip.PRTH1H * cfg->sys.adjust.reg_ia / cfg->sys.adjust.rated_current / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH1H, RN7302_LEN_PRTH1H, tmp_val);
        tmp_val = (cfg->sys.chip.PRTH2L * cfg->sys.adjust.reg_ia / cfg->sys.adjust.rated_current / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH2L, RN7302_LEN_PRTH2L, tmp_val);
        tmp_val = (cfg->sys.chip.PRTH2H * cfg->sys.adjust.reg_ia / cfg->sys.adjust.rated_current / 10) >> 12;
        MeterChipWriteReg(RN7302_REG_PRTH2H, RN7302_LEN_PRTH2H, tmp_val);

        /* 功率分段相位校正清零 */
        MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, 0);
        MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, 0);
        MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, 0);

        /* 写保护2 */
        MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, RN7302_WREN2_CMD_WRDIS2);
    }
    else {
        /* 配置功率分段相位校正 */
        MeterChipWriteReg(RN7302_REG_PA_PHSL, RN7302_LEN_PA_PHSL, cfg->sys.chip.PA_PHSL);
        MeterChipWriteReg(RN7302_REG_PB_PHSL, RN7302_LEN_PB_PHSL, cfg->sys.chip.PB_PHSL);
        MeterChipWriteReg(RN7302_REG_PC_PHSL, RN7302_LEN_PC_PHSL, cfg->sys.chip.PC_PHSL);
        MeterChipWriteReg(RN7302_REG_QA_PHSL, RN7302_LEN_QA_PHSL, cfg->sys.chip.QA_PHSL);
        MeterChipWriteReg(RN7302_REG_QB_PHSL, RN7302_LEN_QB_PHSL, cfg->sys.chip.QB_PHSL);
        MeterChipWriteReg(RN7302_REG_QC_PHSL, RN7302_LEN_QC_PHSL, cfg->sys.chip.QC_PHSL);
        MeterChipWriteReg(RN7302_REG_FPA_PHS, RN7302_LEN_FPA_PHS, cfg->sys.chip.PA_PHSL);
        MeterChipWriteReg(RN7302_REG_FPB_PHS, RN7302_LEN_FPB_PHS, cfg->sys.chip.PB_PHSL);
        MeterChipWriteReg(RN7302_REG_FPC_PHS, RN7302_LEN_FPC_PHS, cfg->sys.chip.PC_PHSL);
        MeterChipWriteReg(RN7302_REG_FQA_PHS, RN7302_LEN_FQA_PHS, cfg->sys.chip.QA_PHSL);
        MeterChipWriteReg(RN7302_REG_FQB_PHS, RN7302_LEN_FQB_PHS, cfg->sys.chip.QB_PHSL);
        MeterChipWriteReg(RN7302_REG_FQC_PHS, RN7302_LEN_FQC_PHS, cfg->sys.chip.QC_PHSL);
    }
}

/**
 * @brief       RN7302电流等级初始化
 * @param[in]    cfg    配置
 * @return      void
 */
static void rn7302_modsel_init(struct msr_config *cfg)
{
    switch (cfg->sys.adjust.rated_current)
    {
    case CURRENT_RATED_C5:
        MeterChipWriteReg(RN7302_REG_IStart_PS, RN7302_LEN_IStart_PS, 0);
        MeterChipWriteReg(RN7302_REG_IStart_Q, RN7302_LEN_IStart_Q, 0);
        break;
    case CURRENT_RATED_K100:
        MeterChipWriteReg(RN7302_REG_IStart_PS, RN7302_LEN_IStart_PS, 0);
        MeterChipWriteReg(RN7302_REG_IStart_Q, RN7302_LEN_IStart_Q, 0);
        break;
    case CURRENT_RATED_K5:
    default:
        MeterChipWriteReg(RN7302_REG_IStart_PS, RN7302_LEN_IStart_PS, 0);
        MeterChipWriteReg(RN7302_REG_IStart_Q, RN7302_LEN_IStart_Q, 0);
        break;
    }
}

/**
 * @brief       RN7302寄存器值检查
 * @param[in]    cfg    配置
 * @return      0 成功；-1 失败
 */
int32_t rn7302_status_check(struct msr_config *cfg)
{
    int32_t reg_hfconst1 = 0;
    int32_t reg_gsua     = 0;
    static int mismatch_count = 0;

    if ((0 != MeterChipReadReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, &reg_hfconst1))
    ||  (0 != MeterChipReadReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, &reg_gsua))) {
        printf("rn7302_status_check: Failed to read HFConst1 or GsUa register\n");
        return -1;
    }

    if ((reg_hfconst1 != cfg->sys.chip.HFConst1)
    ||  (reg_gsua     != cfg->sys.chip.GSUA)) {

        mismatch_count++;
        if (mismatch_count > 3) {
            printf("rn7302_status_check: HFConst1 or GsUa register value mismatch\n");
            printf("  Expected HFConst1=0x%08X, Got=0x%08X\n", cfg->sys.chip.HFConst1, reg_hfconst1);
            printf("  Expected GsUa=0x%08X, Got=0x%08X\n", cfg->sys.chip.GSUA, reg_gsua);
            mismatch_count = 0;
            return -1;
        }
    }
    else {
        mismatch_count = 0;
    }
    
    return 0;
}

/**
 * @brief       RN7302硬件初始化
 * @param[in]    cfg    配置
 * @return      0 成功；-1 失败
 */
void rn7302_init(struct msr_config *cfg)
{
    /* 写使能 */
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, RN7302_WREN_CMD_WREN);

    /* 切换到EMM模式 */
    MeterChipWriteReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, RN7302_WMSW_CMD_GOEMM);

    /* 软件复位 */
    MeterChipWriteReg(RN7302_REG_SOFTRST, RN7302_LEN_SOFTRST, RN7302_SOFTRST_CMD_CHIPRST);

    MeterChipDelayms(100);


    /* 写使能 */
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, RN7302_WREN_CMD_WREN);

    /* 计量控制：使能基波、全波的PQS的电能计量，并参与合相电能计算 */
    MeterChipWriteReg(RN7302_REG_EMUCON, RN7302_LEN_EMUCON, RN7302_EMUCON_VALUE);

    /* 接线模式：根据cfg->wiremode选择三相三线或三相四线接线模式 */
    MeterChipWriteReg(RN7302_REG_MODSEL, RN7302_LEN_MODSEL, (cfg->user.wiremode == WIREMODE_P3W3) ? RN7302_MODSEL_P3W3 : RN7302_MODSEL_P3W4);

    /* 脉冲常数 */
    MeterChipWriteReg(RN7302_REG_HFConst1, RN7302_LEN_HFConst1, cfg->sys.chip.HFConst1);

    /* 电压、电流增益初始化 */
    MeterChipWriteReg(RN7302_REG_GSUA, RN7302_LEN_GSUA, cfg->sys.chip.GSUA);
    MeterChipWriteReg(RN7302_REG_GSUB, RN7302_LEN_GSUB, cfg->sys.chip.GSUB);
    MeterChipWriteReg(RN7302_REG_GSUC, RN7302_LEN_GSUC, cfg->sys.chip.GSUC);
    MeterChipWriteReg(RN7302_REG_GSIA, RN7302_LEN_GSIA, cfg->sys.chip.GSIA);
    MeterChipWriteReg(RN7302_REG_GSIB, RN7302_LEN_GSIB, cfg->sys.chip.GSIB);
    MeterChipWriteReg(RN7302_REG_GSIC, RN7302_LEN_GSIC, cfg->sys.chip.GSIC);
    MeterChipWriteReg(RN7302_REG_GSIN, RN7302_LEN_GSIN, cfg->sys.chip.GSIN);

    /* 电压、电流相位初始化 */
    rn7302_phs_init(cfg);

    /* 配置启动参数：根据电流等级配置 */
    rn7302_modsel_init(cfg);

    /* 配置偏移校正 */
    MeterChipWriteReg(RN7302_REG_IA_OS, RN7302_LEN_IA_OS, cfg->sys.adjust.offset_ia);
    MeterChipWriteReg(RN7302_REG_IB_OS, RN7302_LEN_IB_OS, cfg->sys.adjust.offset_ib);
    MeterChipWriteReg(RN7302_REG_IC_OS, RN7302_LEN_IC_OS, cfg->sys.adjust.offset_ic);

    /* 配置过零阀值 */
    MeterChipWriteReg(RN7302_REG_ZXOT, RN7302_LEN_ZXOT, 2);

    /* 配置相序调整 */
    rn7302_phasecheck_init(cfg);

    /* 配置脉冲模式 */
    rn7302_cfcfg_init(cfg);
    
    /* 切换到EMM模式 */
    MeterChipWriteReg(RN7302_REG_WMSW, RN7302_LEN_WMSW, RN7302_WMSW_CMD_GOEMM);

    /* 清空采样数据缓存区 */
    MeterChipWriteReg(RN7302_REG_WSAVECON, RN7302_LEN_WSAVECON, RN7302_WSAVECON_WSCOMM_CMD_CLEAR);

    /* 等待2秒 */
    MeterChipDelayms(2000);
    
    /* 写使能2 */
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, RN7302_WREN2_CMD_WREN2);

    /* 配置基波/全波有功、无功的SPL复用半波数据 */
    MeterChipWriteReg(RN7302_REG_EMUCFG2, RN7302_LEN_EMUCFG2, RN7302_EMMCFG2_VALUE);

    /* 配置频率、夹角寄存器为1周波更新（约20ms） */
    MeterChipWriteReg(RN7302_REG_EMUCFG3, RN7302_LEN_EMUCFG3, RN7302_EMUCFG3_VALUE);

    /* 写保护2 */
    MeterChipWriteReg(RN7302_REG_WREN2, RN7302_LEN_WREN2, RN7302_WREN2_CMD_WRDIS2);

    /* 写保护 */
    MeterChipWriteReg(RN7302_REG_WREN, RN7302_LEN_WREN, RN7302_WREN_CMD_WRDIS);
}