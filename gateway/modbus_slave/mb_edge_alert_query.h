/*
 * mb_edge_alert_query.h
 * Modbus 0x4F 告警记录查询
 */

#ifndef MB_EDGE_ALERT_QUERY_H
#define MB_EDGE_ALERT_QUERY_H

#include <stdint.h>

#define MB_ALARM_OP_STATS           0x01
#define MB_ALARM_OP_QUERY_PAGE      0x02

#define MB_ALARM_RESULT_OK          0
#define MB_ALARM_RESULT_DB_FAIL     1
#define MB_ALARM_RESULT_SQL_FAIL    2
#define MB_ALARM_RESULT_BAD_PARAM   3

#define MB_ALARM_QUERY_LIMIT_MAX    50

#define MB_ALARM_REQ_STATS_PDU_LEN  20
#define MB_ALARM_REQ_PAGE_PDU_LEN   24
#define MB_ALARM_RSP_HEAD_PDU_LEN   6

/*
 * 处理 0x4F PDU（从 SlaveAddr 起），写入应答 PDU（不含 MBAP/CRC）。
 * 返回 0 成功，<0 表示报文格式错误无需应答。
 */
int mb_edge_alert_query_handle(uint8_t dev_addr,
                               const uint8_t *pdu,
                               uint16_t pdu_len,
                               uint8_t *resp_pdu,
                               uint16_t resp_buf_size,
                               uint16_t *resp_pdu_len);

#endif
