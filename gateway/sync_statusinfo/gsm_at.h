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
/* AT+COPS? → +COPS: 0,0,"CHINA MOBILE",7；oper 写入运营商名，成功返回 0 */
int gsm_at_get_cops(int fd, char *oper, size_t oper_len);

#endif
