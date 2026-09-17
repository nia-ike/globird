/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：db_measure_info.c
*  文件标识：
*  描    述：测量数据刷新序号（写侧）
*
*  当前版本：V1.0
*  作    者：
*  运行环境：
*  操作系统: linux
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <inttypes.h>
#include <unistd.h>
#include "db_measure_info.h"
#include "db_struct.h"
#include "debug.h"


/**
 * @brief : 获取当前时间戳（毫秒）
 * @return: 当前时间戳（毫秒）
 */
static int64_t db_measure_info_utc_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
        return 0;

    return (int64_t)ts.tv_sec * 1000 + (int64_t)(ts.tv_nsec / 1000000);
}

/**
 * @brief : 初始化测量数据刷新序号
 * @param : info: 测量数据刷新序号
 */
void db_measure_info_init(struct db_measure_runtime_info_type *info)
{
    if (info == NULL)
        return;

    memset(info, 0, sizeof(*info));
    info->magic = DB_MEASURE_INFO_MAGIC;
    info->version = DB_MEASURE_INFO_VERSION;
}

/**
 * @brief : 半波数据开始刷新
 * @param : pBase: 测量上下文
 */
void db_measure_info_halfwave_start(struct db_database_measure_base_type *pBase)
{
    uint32_t seq;

    if (pBase == NULL)
        return;

    seq = __atomic_add_fetch(&pBase->info.halfwave_seq, 1u, __ATOMIC_RELEASE);
}

/**
 * @brief : 半波数据结束刷新
 * @param : pBase: 测量上下文
 */
void db_measure_info_halfwave_done(struct db_database_measure_base_type *pBase)
{
    uint32_t seq;

    if (pBase == NULL)
        return;

    __atomic_store_n(&pBase->info.halfwave_utc_seq, db_measure_info_utc_ms(), __ATOMIC_RELAXED);
    seq = __atomic_add_fetch(&pBase->info.halfwave_seq, 1u, __ATOMIC_RELEASE);
}

/**
 * @brief : 全波数据开始刷新
 * @param : pBase: 测量上下文
 */
void db_measure_info_full_cycle_start(struct db_database_measure_base_type *pBase)
{
    uint32_t seq;

    if (pBase == NULL)
        return;

    seq = __atomic_add_fetch(&pBase->info.update_seq, 1u, __ATOMIC_RELEASE);
}

/**
 * @brief : 全波数据结束刷新
 * @param : pBase: 测量上下文
 */
void db_measure_info_full_cycle_done(struct db_database_measure_base_type *pBase)
{
    uint32_t seq;

    if (pBase == NULL)
        return;

    __atomic_store_n(&pBase->info.update_utc_seq, db_measure_info_utc_ms(), __ATOMIC_RELAXED);
    seq = __atomic_add_fetch(&pBase->info.update_seq, 1u, __ATOMIC_RELEASE);
}
