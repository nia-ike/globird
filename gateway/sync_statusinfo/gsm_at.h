#ifndef __GSM_AT_H__
#define __GSM_AT_H__

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

int gsm_at_get_sw_sim_insert(int fd);
int gsm_at_get_at(int fd);
void gsm_at_sw_reset(int fd);
int gsm_at_get_creg(int fd);
int gsm_at_get_csq(int fd);
int gsm_at_get_iccid(int fd, char * iccid);
int gsm_at_get_imei(int fd, char * imei);
int gsm_at_get_cesq(int fd, int *rsrq, int *rsrp);
/* AT+COPS?：oper 为引号内运营商名（可空）；act 为 AcT，缺省写 -1。命令成功返回 0 */
int gsm_at_get_cops(int fd, char *oper, size_t oper_len, int *act);

/* AT+CPIN?：-1 无应答；0 ready；1 pin_locked；2 absent；3 error；4 unknown */
int gsm_at_query_cpin(int fd);

/* AT+CREG?：stat 为注册状态码 0–5，解析成功返回 0 */
int gsm_at_query_creg(int fd, int *stat);

#endif
