/*
 * FCAS 半波测试数据注入：控制区布局与列映射（挂在 db_database_type 末尾）
 * vmem 仅存 magic/state/chip/col_mask；CSV 数据表在 fcas_inject 进程内。
 */
#ifndef __DB_FCAS_INJECT_H__
#define __DB_FCAS_INJECT_H__

#include <stdint.h>
#include "db_measure_realtime.h"

#define DB_FCAS_INJECT_MAGIC        0x46434153u  /* 'FCAS' */
#define DB_FCAS_INJECT_MAX_COLS     19
#define DB_FCAS_INJECT_MAX_ROWS     2048
#define DB_FCAS_INJECT_SLOT_MS      20

enum db_fcas_inject_state {
    DB_FCAS_INJECT_STATE_IDLE    = 0,  /* 不注入，msr 全量发布 */
    DB_FCAS_INJECT_STATE_PLAYING = 1,  /* fcas_inject 正在按行写入半波 SHM */
    DB_FCAS_INJECT_STATE_HOLD    = 2,  /* 播完保持，mask 仍生效，msr 跳过这些列 */
};

/* CSV 列顺序（不含 index），与 fcas_injection.csv 表头一致 */
enum db_fcas_col {
    DB_FCAS_COL_FREQ_HZ = 0,
    DB_FCAS_COL_VA,
    DB_FCAS_COL_VB,
    DB_FCAS_COL_VC,
    DB_FCAS_COL_IA,
    DB_FCAS_COL_IB,
    DB_FCAS_COL_IC,
    DB_FCAS_COL_PA,
    DB_FCAS_COL_PB,
    DB_FCAS_COL_PC,
    DB_FCAS_COL_PTOT,
    DB_FCAS_COL_QA,
    DB_FCAS_COL_QB,
    DB_FCAS_COL_QC,
    DB_FCAS_COL_QTOT,
    DB_FCAS_COL_PFA,
    DB_FCAS_COL_PFB,
    DB_FCAS_COL_PFC,
    DB_FCAS_COL_PFTOT,
};

struct db_fcas_inject_type {
    uint32_t magic;
    uint32_t state;                              /* db_fcas_inject_state */
    uint8_t  chip;                               /* 默认 0 */
    uint8_t  reserved0[3];
    uint8_t  col_mask[DB_FCAS_INJECT_MAX_COLS];  /* 1=注入列，msr 禁止覆盖 */
};

/**
 * 将半波计算结果 src 发布到 dst(halfwave.primary)。
 * 非 IDLE 且 col_mask==1 的列：保留 dst 现有值（由 fcas_inject 写入），不拷贝 src。
 */
void db_fcas_halfwave_publish(struct db_basic_realtime_type *dst,
                              const struct db_basic_realtime_type *src,
                              const struct db_fcas_inject_type *inj);

/** 按列把工程值写入 halfwave.primary（含单位换算） */
void db_fcas_write_col(struct db_basic_realtime_type *dst, int col, float eng);

/** 从 halfwave.primary 读回工程值（日志 CSV） */
float db_fcas_read_col(const struct db_basic_realtime_type *src, int col);

/** 列名（小写，不含 =0/=1） */
const char *db_fcas_col_name(int col);

/** 解析 "freq_hz=1" 类表头，成功返回列号，失败 -1；*enable 取 = 后数字 */
int db_fcas_parse_header_token(const char *token, int *enable);

void db_fcas_inject_clear(struct db_fcas_inject_type *inj);

#endif /* __DB_FCAS_INJECT_H__ */
