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

#include <string.h>
#include "db_measure_info.h"
#include "db_struct.h"
#include <sys/time.h>

void db_measure_info_init(struct db_measure_runtime_info_type *info)
{
    if (info == NULL)
        return;

    if (info->magic == DB_MEASURE_INFO_MAGIC)
        return;

    memset(info, 0, sizeof(*info));
    info->magic = DB_MEASURE_INFO_MAGIC;
    info->version = DB_MEASURE_INFO_VERSION;
}

void db_measure_info_halfwave_start(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_add_fetch(&pBase->info.halfwave_seq, 1u, __ATOMIC_RELEASE);
}

void db_measure_info_halfwave_done(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_add_fetch(&pBase->info.halfwave_seq, 1u, __ATOMIC_RELEASE);
}

void db_measure_info_halfwave_update_utc(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_store_n(&pBase->info.halfwave_utc_seq, time(NULL), __ATOMIC_RELEASE);
}

void db_measure_info_full_cycle_start(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_add_fetch(&pBase->info.update_seq, 1u, __ATOMIC_RELEASE);
}

void db_measure_info_full_cycle_done(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_add_fetch(&pBase->info.update_seq, 1u, __ATOMIC_RELEASE);
}

void db_measure_info_full_cycle_update_utc(struct db_database_measure_base_type *pBase)
{
    if (pBase == NULL)
        return;

    __atomic_store_n(&pBase->info.update_utc_seq, time(NULL), __ATOMIC_RELEASE);
}
