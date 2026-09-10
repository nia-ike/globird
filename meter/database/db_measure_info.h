/*
*  Copyright (c) 2019,珠海科技股份有限公司
*  All rights reserved.
*
*  文件名称：db_measure_info.h
*  文件标识：
*  描    述：测量数据刷新序号（写侧）
*
*  当前版本：V1.0
*  作    者：
*  运行环境：
*  操作系统: linux
*/

#ifndef __DB_MEASURE_INFO_H__
#define __DB_MEASURE_INFO_H__

#include <stdint.h>
#include <time.h>

struct db_database_measure_base_type;

#define DB_MEASURE_INFO_MAGIC     0x444D4941u   /* "DMIA" */
#define DB_MEASURE_INFO_VERSION   1u

struct db_measure_runtime_info_type
{
    uint32_t magic;
    uint32_t version;

    uint32_t update_seq;         /* 全量测量刷新序号 */
    uint32_t halfwave_seq;       /* 半波有效刷新序号（2个连续半波合成1次） */
    
    time_t   update_utc_seq;     /* 全量测量UTC时间戳刷新序号 */
    time_t   halfwave_utc_seq;   /* 半波有效UTC时间戳刷新序号 */
    uint32_t status;             /* 设备状态 */

    uint32_t reserved[9];
};

void db_measure_info_init(struct db_measure_runtime_info_type *info);

void db_measure_info_halfwave_start(struct db_database_measure_base_type *pBase);
void db_measure_info_halfwave_done(struct db_database_measure_base_type *pBase);
void db_measure_info_halfwave_update_utc(struct db_database_measure_base_type *pBase);

void db_measure_info_full_cycle_start(struct db_database_measure_base_type *pBase);
void db_measure_info_full_cycle_done(struct db_database_measure_base_type *pBase);
void db_measure_info_full_cycle_update_utc(struct db_database_measure_base_type *pBase);

#endif
