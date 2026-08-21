#ifndef __DEV_LOG_H__
#define __DEV_LOG_H__

#include <stdint.h>
#include <sys/shm.h>
#include <sys/time.h> 
#include <stdarg.h> 
#include <time.h>
#include <string.h>
#include <sys/ipc.h>
#include <arpa/inet.h>

#define LOG_TX      1
#define LOG_RX      0

#define LOG_LEVEL_INFO     "INFO"
#define LOG_LEVEL_WARN     "WARN"
#define LOG_LEVEL_ERROR    "ERROR"
#define LOG_LEVEL_DEBUG    "DEBUG"
#define LOG_LEVEL_TX       "TX"
#define LOG_LEVEL_RX       "RX"


/* 
    转发通道的调试，首地址从100开始
*/
#define LOG_CHNL_FW(c)                  (c + 100)


typedef int (*log_abort_cb_t)(void *, int);

void *log_open(log_abort_cb_t abort_cb);
void log_close(void);
int log_work_set(int enable, int channlNo, int devNo, char *app_name);
int log_write(int channlNo, int devNo, int dir, uint8_t *pbuf, uint16_t length);
int log_write_string(int channlNo, int devNo, char *level, const char *format, ...);
int log_read(uint8_t *pbuf, uint16_t length);
int log_get_unread_length(void);
int log_buff_size(void);

#endif
