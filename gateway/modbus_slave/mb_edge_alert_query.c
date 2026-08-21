/*
 * mb_edge_alert_query.c
 * Modbus 0x4F 告警记录查询
 */

#include "mb_edge_alert_query.h"
#include "mb_edge_alert_db.h"
#include "mb_com_baseclass.h"
#include "mb_protocol.h"

#include <string.h>

static uint64_t mb_alarm_read_u64_be(const uint8_t *p)
{
    uint64_t v = 0;
    int i;

    for (i = 0; i < 8; i++)
        v = (v << 8) | p[i];

    return v;
}

static uint16_t mb_alarm_read_u16_be(const uint8_t *p)
{
    return (uint16_t)(((uint16_t)p[0] << 8) | p[1]);
}

static void mb_alarm_write_u32_be(uint8_t *p, uint32_t v)
{
    MB_ALIGN_PT_32BIT_WRITE(p, v);
}

static void mb_alarm_write_u16_be(uint8_t *p, uint16_t v)
{
    MB_ALIGN_PT_16BIT_WRITE(p, v);
}

static int mb_alarm_parse_time_range(const uint8_t *pdu, uint16_t pdu_len,
                                     mb_alarm_time_range_t *range)
{
    if (pdu_len < MB_ALARM_REQ_STATS_PDU_LEN)
        return -1;

    range->start_ms = mb_alarm_read_u64_be(pdu + 4);
    range->end_ms = mb_alarm_read_u64_be(pdu + 12);
    return 0;
}

static void mb_alarm_write_rsp_head(uint8_t *resp, uint8_t dev_addr, uint8_t op,
                                    uint8_t result)
{
    resp[0] = dev_addr;
    resp[1] = _FC_ALARM_QUERY;
    resp[2] = op;
    resp[3] = result;
    resp[4] = 0;
    resp[5] = 0;
}

static int mb_alarm_handle_stats(uint8_t dev_addr, const mb_alarm_time_range_t *range,
                                 uint8_t *resp_pdu, uint16_t resp_buf_size,
                                 uint16_t *resp_pdu_len)
{
    uint32_t total = 0;
    int db_ret;

    if (resp_buf_size < MB_ALARM_RSP_HEAD_PDU_LEN + 4)
        return -1;

    db_ret = mb_edge_alert_db_count(range, &total);
    if (db_ret == -1)
    {
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_STATS,
                                MB_ALARM_RESULT_DB_FAIL);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }
    if (db_ret != 0)
    {
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_STATS,
                                MB_ALARM_RESULT_SQL_FAIL);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }

    mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_STATS, MB_ALARM_RESULT_OK);
    mb_alarm_write_u32_be(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN, total);
    *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN + 4;
    return 0;
}

static int mb_alarm_handle_query_page(uint8_t dev_addr,
                                      const mb_alarm_time_range_t *range,
                                      uint16_t offset,
                                      uint16_t limit,
                                      uint8_t *resp_pdu,
                                      uint16_t resp_buf_size,
                                      uint16_t *resp_pdu_len)
{
    char json_buf[MB_COM_TXBUFF_SIZE];
    uint32_t total = 0;
    uint16_t count = 0;
    uint16_t json_len;
    int db_ret;
    uint16_t need_len;
    int json_max;

    if (limit == 0 || limit > MB_ALARM_QUERY_LIMIT_MAX)
    {
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_QUERY_PAGE,
                                MB_ALARM_RESULT_BAD_PARAM);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }

    need_len = MB_ALARM_RSP_HEAD_PDU_LEN + 10;
    if (resp_buf_size <= need_len)
        return -1;

    json_max = (int)resp_buf_size - (int)need_len;
    if (json_max <= 0)
        return -1;

    db_ret = mb_edge_alert_db_query_page(range, offset, limit,
                                         json_buf, json_max,
                                         &count, &total);
    if (db_ret == -1)
    {
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_QUERY_PAGE,
                                MB_ALARM_RESULT_DB_FAIL);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }
    if (db_ret != 0)
    {
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_QUERY_PAGE,
                                MB_ALARM_RESULT_SQL_FAIL);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }

    json_len = (uint16_t)strlen(json_buf);
    need_len = MB_ALARM_RSP_HEAD_PDU_LEN + 10 + json_len;
    if (need_len > resp_buf_size)
        return -1;

    mb_alarm_write_rsp_head(resp_pdu, dev_addr, MB_ALARM_OP_QUERY_PAGE,
                            MB_ALARM_RESULT_OK);
    mb_alarm_write_u32_be(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN, total);
    mb_alarm_write_u16_be(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN + 4, offset);
    mb_alarm_write_u16_be(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN + 6, count);
    mb_alarm_write_u16_be(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN + 8, json_len);
    memcpy(resp_pdu + MB_ALARM_RSP_HEAD_PDU_LEN + 10, json_buf, json_len);

    *resp_pdu_len = need_len;
    return 0;
}

int mb_edge_alert_query_handle(uint8_t dev_addr,
                               const uint8_t *pdu,
                               uint16_t pdu_len,
                               uint8_t *resp_pdu,
                               uint16_t resp_buf_size,
                               uint16_t *resp_pdu_len)
{
    mb_alarm_time_range_t range;
    uint8_t op;

    if (!pdu || !resp_pdu || !resp_pdu_len || pdu_len < 4)
        return -1;

    if (pdu[1] != _FC_ALARM_QUERY)
        return -1;

    op = pdu[2];

    if (mb_alarm_parse_time_range(pdu, pdu_len, &range) != 0)
        return -1;

    switch (op)
    {
    case MB_ALARM_OP_STATS:
        return mb_alarm_handle_stats(dev_addr, &range, resp_pdu, resp_buf_size,
                                     resp_pdu_len);

    case MB_ALARM_OP_QUERY_PAGE:
    {
        uint16_t offset;
        uint16_t limit;

        if (pdu_len < MB_ALARM_REQ_PAGE_PDU_LEN)
            return -1;

        offset = mb_alarm_read_u16_be(pdu + 20);
        limit = mb_alarm_read_u16_be(pdu + 22);
        return mb_alarm_handle_query_page(dev_addr, &range, offset, limit,
                                          resp_pdu, resp_buf_size, resp_pdu_len);
    }

    default:
        mb_alarm_write_rsp_head(resp_pdu, dev_addr, op, MB_ALARM_RESULT_BAD_PARAM);
        *resp_pdu_len = MB_ALARM_RSP_HEAD_PDU_LEN;
        return 0;
    }
}
