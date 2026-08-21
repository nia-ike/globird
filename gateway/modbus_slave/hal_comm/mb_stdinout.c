/*
* Copyright (c) 
* All rights reserved.
* 
* 文件名称：
* 文件标识：
* 描    述：
* 
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录 
*  作者       时间         版本      修改描述
*  chenzy   2020.6.09   V1.00      创建
*/

#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <fcntl.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <linux/serial.h>
#include <sys/ioctl.h>

#include "mb_com_baseclass.h"



static struct termios tio0, tiosfd, tio;


#define TERMIOS_CLEAR_ISIG      (1 << 0)
#define TERMIOS_RAW_CRNL_INPUT  (1 << 1)
#define TERMIOS_RAW_CRNL_OUTPUT (1 << 2)
#define TERMIOS_RAW_CRNL        (TERMIOS_RAW_CRNL_INPUT|TERMIOS_RAW_CRNL_OUTPUT)
#define TERMIOS_RAW_INPUT       (1 << 3)


/*
    this CPU uart fifo == 64
*/
#define HARDWARE_FIFO_SIZE      64


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int get_termios_and_make_raw(int fd, struct termios *newterm, struct termios *oldterm, int flags)
{
//TODO: slattach, shell read might be adapted to use this too: grep for "tcsetattr", "[VTIME] = 0"
	int r;

	memset(oldterm, 0, sizeof(*oldterm)); /* paranoia */
	r = tcgetattr(fd, oldterm);
	*newterm = *oldterm;

	/* Turn off buffered input (ICANON)
	 * Turn off echoing (ECHO)
	 * and separate echoing of newline (ECHONL, normally off anyway)
	 */
	newterm->c_lflag &= ~(ICANON | ECHO | ECHONL);
	if (flags & TERMIOS_CLEAR_ISIG) {
		/* dont recognize INT/QUIT/SUSP chars */
		newterm->c_lflag &= ~ISIG;
	}
	/* reads will block only if < 1 char is available */
	newterm->c_cc[VMIN] = 1;
	/* no timeout (reads block forever) */
	newterm->c_cc[VTIME] = 0;
/* IXON, IXOFF, and IXANY:
 * IXOFF=1: sw flow control is enabled on input queue:
 * tty transmits a STOP char when input queue is close to full
 * and transmits a START char when input queue is nearly empty.
 * IXON=1: sw flow control is enabled on output queue:
 * tty will stop sending if STOP char is received,
 * and resume sending if START is received, or if any char
 * is received and IXANY=1.
 */
	if (flags & TERMIOS_RAW_CRNL_INPUT) {
		/* IXON=0: XON/XOFF chars are treated as normal chars (why we do this?) */
		/* dont convert CR to NL on input */
		newterm->c_iflag &= ~(IXON | ICRNL);
	}
	if (flags & TERMIOS_RAW_CRNL_OUTPUT) {
		/* dont convert NL to CR+NL on output */
		newterm->c_oflag &= ~(ONLCR);
		/* Maybe clear more c_oflag bits? Usually, only OPOST and ONLCR are set.
		 * OPOST  Enable output processing (reqd for OLCUC and *NL* bits to work)
		 * OLCUC  Map lowercase characters to uppercase on output.
		 * OCRNL  Map CR to NL on output.
		 * ONOCR  Don't output CR at column 0.
		 * ONLRET Don't output CR.
		 */
	}
	if (flags & TERMIOS_RAW_INPUT) {
#ifndef IMAXBEL
# define IMAXBEL 0
#endif
#ifndef IUCLC
# define IUCLC 0
#endif
#ifndef IXANY
# define IXANY 0
#endif
		/* IXOFF=0: disable sending XON/XOFF if input buf is full
		 * IXON=0: input XON/XOFF chars are not special
		 * BRKINT=0: dont send SIGINT on break
		 * IMAXBEL=0: dont echo BEL on input line too long
		 * INLCR,ICRNL,IUCLC: dont convert anything on input
		 */
		newterm->c_iflag &= ~(IXOFF|IXON|IXANY|BRKINT|INLCR|ICRNL|IUCLC|IMAXBEL);
	}
	return r;
}


// set raw tty mode
/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void xget1(int fd, struct termios *t, struct termios *oldt)
{
	get_termios_and_make_raw(fd, t, oldt, 0
		| TERMIOS_CLEAR_ISIG /* ^C is ASCII char 3, not "interrupt me!" */
		| TERMIOS_RAW_INPUT /* pass all chars verbatim, no special handling or translating CR->NL */
		| TERMIOS_RAW_CRNL  /* dont convert NL<->CR on output too */
	);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int xset1(int fd, struct termios *tio, const char *device)
{
	int ret = tcsetattr(fd, TCSAFLUSH, tio);

	return ret;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_serial_close(struct mb_comm_ctx_t *pCtx)
{
    if (isatty(STDIN_FILENO))
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio0);
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_init(struct mb_comm_ctx_t *pCtx)
{
    return 0;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_connect(struct mb_comm_ctx_t *pCtx)
{
    if (isatty(STDIN_FILENO)) {
		xget1(STDIN_FILENO, &tio, &tio0);
		if (xset1(STDIN_FILENO, &tio, "stdin"))
			return -1;
	}

    return 0;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void mb_serial_disconnect(struct mb_comm_ctx_t *pCtx)
{
    if (isatty(STDIN_FILENO))
		tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio0);

}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_read(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
	fd_set rd;
    struct timeval tv ;
    int32_t iRead;
    uint16_t wPos;

    /* Packet position */
    wPos = 0;
    /* Packet */
    // 等待时间
    tv.tv_sec   = timeout / 1000;
    tv.tv_usec  = timeout % 1000 * 1000;
    while((wPos < bytes) )
    {
        FD_ZERO(&rd);
        FD_SET(STDIN_FILENO, &rd);

        if(select(STDIN_FILENO+1, &rd, NULL, NULL, &tv) <= 0)
        {
            break;
        }
        else
        {
            iRead = read(STDIN_FILENO, pbuf + wPos, bytes - wPos);
            if(iRead > 0)
            {
                tv.tv_sec   = 0 ;
                tv.tv_usec  = ((1 * 11 * 1000 * HARDWARE_FIFO_SIZE * 1000) / 921600) ; // baudrate 921600
                wPos += iRead;
            }
            else 
            {
                mb_serial_disconnect(pCtx);
        		exit(1);
            }
        }
    }

    if(pbuf[0] == 'q')
    {
        mb_serial_disconnect(pCtx);
        system("touch /tmp/config_exit");
        exit(1);
    }


    return wPos;

#if 0
    int n;

    n = read(STDIN_FILENO, pbuf, bytes);
    if(n <= 0)
        return -1;

    if(pbuf[0] == 'q')
    {
        mb_serial_disconnect(pCtx);
        exit(1);
    }

    return n;
#endif
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_write(struct mb_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    return write(STDOUT_FILENO, pbuf, bytes);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_Flush(struct mb_comm_ctx_t *pCtx)
{
    if (isatty(STDIN_FILENO))
		return tcsetattr(STDIN_FILENO, TCSAFLUSH, &tio0);

    return 0;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int mb_serial_setoption(struct mb_comm_ctx_t *pCtx, int cmd)
{
    switch(cmd)
    {
        case MB_COM_SET_SERIAL_TERMIOS:
        break;
    }
}


static struct mb_com_baseclass_ops_t stdinout_ops = {
    .init = mb_serial_init,
    .connect = mb_serial_connect,
    .disconnect = mb_serial_disconnect,
    .read = mb_serial_read,
    .write = mb_serial_write,
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct mb_com_baseclass_ops_t *mb_com_stdinout_ops_get(void)
{
    return &stdinout_ops;
}