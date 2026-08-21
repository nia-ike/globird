/*
 * mb_edge_alert_db.h
 * edge_alert 告警库只读访问（modbus_slave 侧）
 */

#ifndef MB_EDGE_ALERT_DB_H
#define MB_EDGE_ALERT_DB_H

#include <stdint.h>

#define MB_EDGE_ALERT_DB_PATH    "/var/run/record/edge_alert.db"

typedef struct {
    uint64_t start_ms;
    uint64_t end_ms;
} mb_alarm_time_range_t;

/* 0=成功, -1=DB不可用, -2=SQL失败 */
int mb_edge_alert_db_count(const mb_alarm_time_range_t *range, uint32_t *total);

/*
 * 分页查询并序列化为 JSON（{"records":[...]}）。
 * json_buf 由调用方提供；count_out 为实际写入条数，total_out 为命中总数。
 */
int mb_edge_alert_db_query_page(const mb_alarm_time_range_t *range,
                                uint16_t offset,
                                uint16_t limit,
                                char *json_buf,
                                int json_buf_size,
                                uint16_t *count_out,
                                uint32_t *total_out);

#endif
