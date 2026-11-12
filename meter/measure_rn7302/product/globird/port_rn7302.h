/**
 * @file        port_rn7302.h
 * @brief       measure_rn7302 ↔ RN7302 设备文件对接（misc fops）
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   端口 RN7302
 *  2026-10-29  yansh    1.0.1   移植 Rn7302File misc 读写接口
 */
#ifndef PORT_RN7302_H
#define PORT_RN7302_H

#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* rn7302 misc 读错误：与驱动 errno 对齐 */
#define RN7302_ERR_SPI   ECOMM     /* SPI 通信失败 */
#define RN7302_ERR_CRC   EBADMSG   /* CRC 校验失败 */

static inline int rn7302_is_comm_error(int err)
{
    return (err == RN7302_ERR_SPI || err == RN7302_ERR_CRC);
}

/**
 * @brief       毫秒级延时
 * @param[in]   ms    延时毫秒数
 * @return      无
 */
void MeterChipDelayms(uint32_t ms);

/**
 * @brief       打开 RN7302 misc 设备
 * @param[in]   chip  芯片号（对应 /dev/rn7302-0.<chip>）
 * @return      成功返回 fd（>=0）；失败 -1
 */
int MeterChipInit(int chip);

/**
 * @brief       关闭 RN7302 设备
 */
void MeterChipDestory(void);

/**
 * @brief       读寄存器
 * @param[in]   reg    寄存器地址（作 lseek 偏移）
 * @param[in]   nbyte  字节数
 * @param[out]  pval   读出值
 * @return      0 成功；-1 失败
 */
int MeterChipReadReg(uint32_t reg, uint32_t nbyte, int32_t *pval);

/**
 * @brief       写寄存器
 * @param[in]   reg    寄存器地址
 * @param[in]   nbyte  字节数
 * @param[in]   val    写入值
 * @return      0 成功；-1 失败
 */
int MeterChipWriteReg(uint32_t reg, uint32_t nbyte, uint32_t val);

/**
 * @brief       突发读寄存器区（驱动固定 8 寄存器 / 32 字节）
 * @param[in]   reg    起始寄存器地址
 * @param[in]   nbyte  须为 32
 * @param[out]  pval   读出缓冲
 * @return      成功返回 32；失败 -1
 */
int32_t MeterChipReadArea(uint32_t reg, uint32_t nbyte, uint32_t *pval);

/**
 * @brief       当前设备路径（未初始化返回 NULL）
 */
const char *MeterChipGetDevicePath(void);

/**
 * @brief       当前设备 fd（未初始化返回 -1）
 */
int MeterChipGetFd(void);

#ifdef __cplusplus
}
#endif

#endif
