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

void *log_open(void);
void log_close(void);
int log_work_set(int enable, int channlNo, int devNo);
int log_write(int channlNo, int devNo, int dir, uint8_t *pbuf, uint16_t length);
int log_read(uint8_t *pbuf, uint16_t length);
int log_get_unread_length(void);


#endif
