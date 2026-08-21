#ifndef __GSM_SERIAL_H__
#define __GSM_SERIAL_H__


#include <features.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <string.h>
#include <paths.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>


int indexOfBaud(int baudrate);
int open_serialport_withbaud(char *dev, speed_t baud);
int open_serialport(char *dev);
void setAdvancedOptions(int fd, speed_t baud) ;
void close_serialport(int fd);



#endif
