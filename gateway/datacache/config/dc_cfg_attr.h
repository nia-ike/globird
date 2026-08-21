/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/


#ifndef __DC_CFG_ATTR_H__
#define __DC_CFG_ATTR_H__

#include <stdint.h>

#include "db_struct.h"
#include "dc_cfg_json.h"

/* 缓存模式：滚动续传 / 按日归档（二选一） */
#define DC_CACHE_MODE_ROLL      0
#define DC_CACHE_MODE_DAILY     1

/*
 * 旧 JSON 无新字段时的兼容默认（现场旧设备升级新程序）
 *  - 仅滚动续传
 *  - 高水位 8MB（原 DC_SQL_CACHE_FILE_MAXSIZE）
 *  - 低水位 6MB（旧逻辑无低水位，给滚动裁剪用的保守默认）
 */
#define DC_CFG_LEGACY_MODE              DC_CACHE_MODE_ROLL
#define DC_CFG_LEGACY_HIGH_SIZE         (8u * 1024u * 1024u)
#define DC_CFG_LEGACY_LOW_SIZE          (6u * 1024u * 1024u)
#define DC_CFG_LEGACY_HISTORY_DAYS      8
#define DC_CFG_LEGACY_HISTORY_PATH      "/userdata/history"

/* 新配置显式填写时的推荐默认 */
#define DC_CFG_DEFAULT_HIGH_SIZE        (10u * 1024u * 1024u)
#define DC_CFG_DEFAULT_LOW_SIZE         (9u * 1024u * 1024u)


struct datacache_attr_t
{
    uint8_t checked;

    uint16_t max_items;
    uint16_t max_rows;
    uint16_t max_alarm_nums;

    uint32_t inv_time;

    /* ---- 以下为扩展字段；旧 JSON 缺失时保持 LEGACY_* ---- */
    uint8_t  mode;          /* DC_CACHE_MODE_ROLL / DAILY */
    uint32_t high_size;     /* 高水位字节数 */
    uint32_t low_size;      /* 低水位字节数（仅 roll 模式裁剪用） */
    uint16_t history_days;  /* 按日模式 NAND 保留天数 */
    char     history_path[128];
};


struct datacache_ctx_t
{
    uint8_t                 debug;
    struct datacache_attr_t attr;
    struct db_spl_realtime_data_tbl_t *pDB;
};


int dc_cfg_load(struct datacache_ctx_t *pCtx, char *file);

#endif
