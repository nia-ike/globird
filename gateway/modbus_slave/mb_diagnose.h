/*
 * mb_diagnose.h
 * Modbus 0x50 现场诊断（结构化，非任意 shell）
 */

#ifndef MB_DIAGNOSE_H
#define MB_DIAGNOSE_H

#include <stdint.h>

/* Op */
#define MB_DIAG_OP_CAPS             0x01
#define MB_DIAG_OP_NETIF_LIST       0x10
#define MB_DIAG_OP_ROUTE_LIST       0x11
#define MB_DIAG_OP_CONN_LIST        0x12
#define MB_DIAG_OP_RES_SUMMARY      0x13
#define MB_DIAG_OP_DNS_RESOLVE      0x20
#define MB_DIAG_OP_TCP_PROBE        0x21
#define MB_DIAG_OP_JOB_START        0x30
#define MB_DIAG_OP_JOB_STATUS       0x31
#define MB_DIAG_OP_JOB_RESULT       0x32
#define MB_DIAG_OP_JOB_CANCEL       0x33
#define MB_DIAG_OP_JOB_LIST         0x34
/* 临时串口旁路透传（COM1~8 / TCP Server） */
#define MB_DIAG_OP_BYPASS_START     0x40
#define MB_DIAG_OP_BYPASS_STOP      0x41
#define MB_DIAG_OP_BYPASS_QUERY     0x42

/* result */
#define MB_DIAG_RESULT_OK           0
#define MB_DIAG_RESULT_UNSUPPORTED  1
#define MB_DIAG_RESULT_BAD_PARAM    2
#define MB_DIAG_RESULT_BUSY         3
#define MB_DIAG_RESULT_EXEC_FAIL    4
#define MB_DIAG_RESULT_NO_JOB       5
#define MB_DIAG_RESULT_NOT_READY    6
#define MB_DIAG_RESULT_DENIED       7
#define MB_DIAG_RESULT_INTERNAL     8

#define MB_DIAG_REQ_HEAD_LEN        4
#define MB_DIAG_RSP_HEAD_LEN        8

#define MB_DIAG_HOST_LEN_MAX        253
#define MB_DIAG_PING_COUNT_MAX      10
#define MB_DIAG_PING_TIMEOUT_MS_MAX 5000
#define MB_DIAG_TCP_TIMEOUT_MS_MAX  5000
#define MB_DIAG_DNS_TIMEOUT_MS_MAX  3000
#define MB_DIAG_RESULT_CHUNK_MAX    1024
#define MB_DIAG_JOBS_MAX            2
#define MB_DIAG_JOB_TTL_SEC         120

/*
 * 处理 0x50 PDU（从 SlaveAddr 起），写入应答 PDU（不含 MBAP/CRC）。
 * 返回 0 成功组应答，<0 表示报文格式错误无需应答。
 */
int mb_diagnose_handle(uint8_t dev_addr,
                       const uint8_t *pdu,
                       uint16_t pdu_len,
                       uint8_t *resp_pdu,
                       uint16_t resp_buf_size,
                       uint16_t *resp_pdu_len);

#endif
