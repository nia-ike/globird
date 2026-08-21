/*
* Copyright (c) 2013,珠海一构物联有限公司
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

#include "com_baseclass.h"


/*
    this CPU uart fifo == 64
*/
#define HARDWARE_FIFO_SIZE      64

#define SET_FLAGS_FROM_STATE(flags, bit, state)		\
	do {						\
        if (state == true)	\
            flags |= bit;		\
        else				\
            flags &= ~(bit);	\
	} while (0);




/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_serial_init(struct hal_comm_ctx_t *pCtx)
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
static int hal_serial_comset(struct hal_comm_ctx_t *pCtx)
{
    struct termios     tios;
    int32_t            speed;


    // If we do not have a valid file descriptor then return with failure.
    if (-1 == pCtx->fd)
        return -1 ;

    memset(&tios, 0, sizeof(struct termios));

    /* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    
    switch (pCtx->baud) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;

    }

    /* Set the baud rate */
    if ((cfsetispeed(&tios, speed) < 0) ||
        (cfsetospeed(&tios, speed) < 0)) {
        close(pCtx->fd);
        pCtx->fd = -1;
        return -1;
    }

    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (pCtx->databit) {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (pCtx->stopbit == 1)
        tios.c_cflag &=~ CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (pCtx->paritybit == 'N') {
        /* None */
        tios.c_cflag &=~ PARENB;
    } else if (pCtx->paritybit == 'E') {
        /* Even */
        tios.c_cflag |= PARENB;
        tios.c_cflag &=~ PARODD;
    } else {
        /* Odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (pCtx->paritybit == 'N') {
        /* None */
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &=~ OPOST;

    /* C_CC         Control characters
       VMIN         Minimum number of characters to read
       VTIME        Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
    */
    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(pCtx->fd, TCSANOW, &tios) < 0) {
        close(pCtx->fd);
        pCtx->fd = -1;
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
static int hal_serial_connect(struct hal_comm_ctx_t *pCtx)
{
    int ret;
    // struct serial_rs485 rs485conf;

    if(!pCtx->dev)
    {
        printf("serial dev is NULL\n");
        return -1;
    }

    if(pCtx->fd < 0)
    {

        // Since we are dealing with the serial port we need to use the O_NOCTTY option.
		// Try to open the serial port.
		if ((pCtx->fd = open(pCtx->dev, O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL)) != -1)
        {
            
            //    ret = ioctl(pCtx->fd, TIOCGRS485, &rs485conf);
            //    /* this uart is working in RS485 mode */
            //    SET_FLAGS_FROM_STATE(rs485conf.flags, SER_RS485_ENABLED, true);
            //    SET_FLAGS_FROM_STATE(rs485conf.flags, SER_RS485_RTS_ON_SEND, true);
            //    SET_FLAGS_FROM_STATE(rs485conf.flags, SER_RS485_RTS_AFTER_SEND, false);
            //    SET_FLAGS_FROM_STATE(rs485conf.flags, SER_RS485_RX_DURING_TX, false);
            //    rs485conf.delay_rts_before_send = 0;
            //    rs485conf.delay_rts_after_send  = 0;
            //    ret = ioctl(pCtx->fd, TIOCSRS485, &rs485conf);
            

            /* initial uart */
            ret = hal_serial_comset(pCtx);
            if(ret < 0)
                return -1;

            fcntl(pCtx->fd, F_SETFL, FNDELAY) ;

            return 0;
        }
    }

    return -1;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void hal_serial_disconnect(struct hal_comm_ctx_t *pCtx)
{
    if(pCtx->fd < 0)
        return;

    shutdown(pCtx->fd, SHUT_RDWR);
    close(pCtx->fd);
    pCtx->fd = -1;
}




/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_serial_read(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes, int timeout)
{
    fd_set rd;
    struct timeval tv ;
    int32_t iRead;
    uint16_t wPos;

	if (-1 == pCtx->fd)
		return -1 ;


    /* Packet position */
    wPos = 0;
    /* Packet */
    // 等待时间
    tv.tv_sec   = timeout / 1000;
    tv.tv_usec  = timeout % 1000 * 1000;

    while((wPos < bytes) )
    {
        FD_ZERO(&rd);
        FD_SET(pCtx->fd, &rd);

        if(select(pCtx->fd+1, &rd, NULL, NULL, &tv) <= 0)
        {
            break;
        }
        else
        {
            iRead = read(pCtx->fd, pbuf + wPos, bytes - wPos);
            if(iRead > 0)
            {
                tv.tv_sec   = 0 ;
                tv.tv_usec  = ((1 * 11 * 1000 * HARDWARE_FIFO_SIZE * 1000) / pCtx->baud) ;
                wPos += iRead;
            }
            else 
            {
                close(pCtx->fd);
                pCtx->fd = -1;
                return -1;
            }
        }
    }
    
    if(wPos > 0)
    {
        MB_PACKET_DUMP(pCtx, "RX:", pbuf, wPos);
    }
    return wPos;
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_serial_write(struct hal_comm_ctx_t *pCtx, uint8_t *pbuf, uint16_t bytes)
{
    if(-1 == pCtx->fd)
        return -1;
    
    MB_PACKET_DUMP(pCtx, "TX:", pbuf, bytes);
    return write(pCtx->fd, pbuf, bytes);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_serial_Flush(struct hal_comm_ctx_t *pCtx)
{
    if(-1 == pCtx->fd)
        return -1;
    
    return tcflush(pCtx->fd, TCIOFLUSH);
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static int hal_serial_setoption(struct hal_comm_ctx_t *pCtx, int cmd)
{
    switch(cmd)
    {
        case HAL_COM_SET_SERIAL_TERMIOS:
            hal_serial_comset(pCtx);
        break;
        default:
            return -1;
    }

    return 0;
}



static struct hal_com_baseclass_ops_t serial_ops = {
    .init = hal_serial_init,
    .connect = hal_serial_connect,
    .disconnect = hal_serial_disconnect,
    .read = hal_serial_read,
    .write = hal_serial_write,
    .flush = hal_serial_Flush
};

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
struct hal_com_baseclass_ops_t *hal_com_serial_ops_get(void)
{
    return &serial_ops;
}