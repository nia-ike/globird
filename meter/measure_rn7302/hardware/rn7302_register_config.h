/**
 * @file        rn7302_regsiter_config.h
 * @brief       RN7302 寄存器配置常量 / 位域 / 命令字
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   从 measure_process/Rn7302.h 誊写并补注释
 *
 * 来源对照：
 *  - measure_process/input_hal/Rn7302.h（约 961–1068 行）
 *  - 锐能微 RN830x/RN730x V3 用户手册：配置与状态寄存器（WMSW/MODSEL/
 *    EMUCON/CFCFG/EMUCFG/EMMIE 等）
 *
 * 说明：
 *  - 本文件只定义「写寄存器时用到的取值 / 位掩码 / 命令」，
 *    寄存器地址与长度见 rn7302_register.h（RN7302_REG_xxx / RN7302_LEN_xxx）。
 *  - 宏名尽量与旧工程保持一致，便于对照移植。
 */
#ifndef RN7302_REGISTER_CONFIG_H
#define RN7302_REGISTER_CONFIG_H

/**
 * WREN：
 *  - WREN-0xE5：写使能
 *  - WRDIS-0xDC：写保护
 */
#define RN7302_WREN_CMD_WREN        0xE5
#define RN7302_WREN_CMD_WRDIS       0xDC

/**
 * WREN2：
 *  - WREN2-0xAA：写使能2
 *  - bit23-16：PhaseAdjust1
 */
#define RN7302_WREN2_CMD_WREN2       ((RN7302_WREN_CMD_WREN << 8) | 0xAA)
#define RN7302_WREN2_CMD_WRDIS2      ((RN7302_WREN_CMD_WRDIS << 8) | 0xAA)

/**
 * WMSW：
 *  - GOSLM-0x18：若系统处于EMM/NVM1/NVM2，使芯片切换到SLM
 *  - GONVM2-0x5C：若系统处于SLM/EMM/NVM1，是芯片切换到NVM2并完成一次NVM2电流比较判断
 *  - GONVM1-0xE1：若系统处于SLM/NVM2，是芯片产生唤醒复位，复位后切换到NVM1并启动NVM1电流有效值计算
 *  - GOEMM-0xA2：若系统处于SLM/NVM2，是芯片产生复位，复位后芯片切换到EMM并启动EMM下所有功能
 */
#define RN7302_WMSW_CMD_GOSLM   0x18
#define RN7302_WMSW_CMD_GONVM2  0x5C
#define RN7302_WMSW_CMD_GONVM1  0xE1
#define RN7302_WMSW_CMD_GOEMM   0xA2

/**
 * SOFTRST：
 *  - 0xFA：软件复位
 *  - 0x69：软件复位EMU（包括EMU、NVM）计算模块
 *  - 0xB4：软件复位EMU（包括EMU、NVM）计算模块
 *  - 0xD2：软件复位FLK模块寄存器和计算模块
 *  - 0xE1：软件复位MMU波形缓存DMA状态机
 *  - 0x87：软件复位SPI状态机
 *  - 0x4C：软件复位HSDC状态机
 *  - 0xA5：软件复位ECT（配置寄存器、状态机）
 */
#define RN7302_SOFTRST_CMD_CHIPRST  0xFA

/**
 * EMUCON：
 *  - PRUN[2:0]：C/B/A相有功计量使能位。=1使能该相有功电能计量，且该相有功参与合相有功电能计算；=0关闭该相有功电能计量，且该相有功不参与合相有功电能计算
 *  - QRUN[6:4]：C/B/A相无功计量使能位。=1使能该相无功电能计量，且该相无功参与合相无功电能计算；=0关闭该相无功电能计量，且该相无功不参与合相无功电能计算
 *  - SRUN[10:8]：C/B/A相视在计量使能位。=1使能该相视在电能计量，且该相视在参与合相视在电能计算；=0关闭该相视在电能计量，且该相视在不参与合相视在电能计算
 *  - FPRUN[16:14]：C/B/A相基波有功计量使能位。=1使能该相基波有功电能计量，且该相基波有功参与合相基波有功电能计算；=0关闭该相基波有功电能计量，且该相基波有功不参与合相基波有功电能计算
 *  - FQRUN[20:18]：C/B/A相基波无功计量使能位。=1使能该相基波无功电能计量，且该相基波无功参与合相基波无功电能计算；=0关闭该相基波无功电能计量，且该相基波无功不参与合相基波无功电能计算
 *  - FSRUN[24:22]：C/B/A相基波视在计量使能位。=1使能该相基波视在电能计量，且该相基波视在参与合相基波视在电能计算；=0关闭该相基波视在电能计量，且该相基波视在不参与合相基波视在电能计算
 */
#define RN7302_EMUCON_PRUN          (0x07 << 0)
#define RN7302_EMUCON_QRUN          (0x07 << 4)
#define RN7302_EMUCON_SRUN          (0x07 << 8)
#define RN7302_EMUCON_FPRUN         (0x07 << 12)
#define RN7302_EMUCON_FQRUN         (0x07 << 16)
#define RN7302_EMUCON_FSRUN         (0x07 << 20)
#define RN7302_EMUCON_VALUE         (RN7302_EMUCON_PRUN | RN7302_EMUCON_QRUN | RN7302_EMUCON_SRUN | RN7302_EMUCON_FPRUN | RN7302_EMUCON_FQRUN | RN7302_EMUCON_FSRUN)

/**
 * MODSEL：
 *  - 0x33：三相三线接线模式
 *  - 0x00：三相四线接线模式
 */
#define RN7302_MODSEL_P3W3          0x33
#define RN7302_MODSEL_P3W4          0x00

/**
 * CFCFG：
 *  - bit0-CF1CFG：有功脉冲选择
 *  - bit1-CF1CONST：有功脉冲常数选择
 *  - bit4-CF2CFG：无功脉冲选择
 *  - bit5-CF2CONST：无功脉冲常数选择
 *  - bit8-CF3CFG：视在脉冲选择
 *  - bit9-CF3CONST：视在脉冲常数选择
 */
#define RN7302_CFCFG_CF1CFG_P       0x043770
#define RN7302_CFCFG_CF1CFG_Q       0x043771
#define RN7302_CFCFG_CF1CFG_FP      0x043773
#define RN7302_CFCFG_CF1CFG_FQ      0x043774

/**
 * WSAVECON：
 *  - WSCOMM-0x01：缓冲区清零
 */
#define RN7302_WSAVECON_WSCOMM_CMD_CLEAR    (1 << 4)

/**
 * EMMCFG2：
 *  - bit3-P_HW_CFG：  半波有功功率选择全波电压过零点计算
 *  - bit29-Q_HW_SEL： 全波无功电能计算采用半周期更新的基波无功功率积分；同时SPL_Qx输出半周期更新的全波无功功率
 *  - bit30-FP_HW_SEL：基波有功电能计算采用半周期更新的基波有功功率积分；同时SPL_FPx输出半周期更新的全波有功功率
 *  - bit31-FQ_HW_SEL：基波无功电能计算采用半周期更新的基波无功功率积分；同时SPL_FQx输出半周期更新的全波无功功率
 */
#define RN7302_EMMCFG2_VALUE        0xE0000008

/**
 * EMUCFG3：
 *  - bit4-ANGLE_FREQ：夹角更新 1 周波  ≈20ms
 *  - bit21-Freq_Cycle：UFreq 更新 1 周波
 */
#define RN7302_EMUCFG3_VALUE        0x00200010

#endif /* RN7302_REGISTER_CONFIG_H */
