/**
 * @file        msr_process.c
 * @brief       测量处理
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   测量处理
 */
#include "msr_process.h"
#include "algorithm_fullwave.h"
#include "algorithm_fundwave.h"
#include "algorithm_halfwave.h"

/**
 * @brief       测量处理
 * @param[in]    cfg     配置
 * @param[in,out] data   数据
 * @param[in]    in      输入
 * @return      0 成功；-1 失败
 */
int msr_process_fullfond(struct msr_config *cfg, struct msr_data *data, struct msr_input *in)
{
    struct msr_energy_realtime eng_full = {0};

    if (cfg == NULL || data == NULL || in == NULL) {
        return -1;
    }

    if (0 != alg_fullwave(&data->full, cfg, &in->full, &eng_full, &in->energy)) {
        return -1;
    }

    return 0;
}

/**
 * @brief       半波测量处理
 * @param[in]    cfg     配置
 * @param[in,out] data   数据
 * @param[in]    in      输入
 * @return      0 成功；-1 失败
 */
int msr_process_halfwave(struct msr_config *cfg, struct msr_data *data, struct msr_input *in)
{
    struct msr_halfwave_volt_state st_v = {0};
    struct msr_halfwave_curr_state st_i = {0};

    if (cfg == NULL || data == NULL || in == NULL) {
        return -1;
    }

    if (0 != alg_halfwave(&data->half, cfg, &in->full, &st_v, &st_i, 0)) {
        return -1;
    }

    return 0;
}