#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <termios.h>
#include "gsm_at.h"
#include "gsm_serial.h"

#define BUF_SIZE	512

/* Sends an AT-command to a given serial port and waits
 * for reply.
 *
 * PARAMS:
 * fd  - file descriptor
 * cmd - command
 * to  - how many microseconds to wait for response (this is done 100 times)
 * RETURNS:
 * 1 on success (OK-response), 0 otherwise
 */
static int gsm_at_command(int fd, char *cmd, char *respond1, char *respond2, int to)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i;
    int returnCode = 0;
    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, cmd, strlen(cmd));

    for (i = 0; i < 50; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = to;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {
            if (FD_ISSET(fd, &rfds))
            {
                /* Create '\0' for strstr,  instead of memset(buf, 0 ,sizeof(buf));   */
                buf[0] = 0;
                len = read(fd, buf, sizeof(buf) - 1);
                if(len <= 0){
                    break;
                }
                buf[len] = 0;
                if(respond1 != NULL)
                {
                    if (strstr((char *)buf, respond1) != NULL) {
                        returnCode = 1;
                        break;
                    }
                }

                if(respond2 != NULL)
                {
                    if (strstr((char *)buf, respond2) != NULL) {
                        returnCode = 1;
                        break;
                    }
                }

                if (strstr((char *)buf, "ERROR") != NULL) {
                    break;
                }

            }
        }
    }

    return returnCode;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_sw_sim_insert(int fd)
{
    return gsm_at_command(fd, (char *)"at+cpin?\r\n", (char *)"OK", NULL, 20000);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_at(int fd)
{
    return gsm_at_command(fd, (char *)"at\r\n", (char *)"OK", NULL, 20000);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_creg(int fd)
{
    return gsm_at_command(fd, (char *)"at+creg?\r\n", (char *)"+CREG: 0,1", (char *)"+CREG: 0,5", 20000);
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
void gsm_at_sw_reset(int fd)
{
    gsm_at_command(fd, (char *)"at+cfun=0\r\n", (char *)"OK", NULL, 20000);
    sleep(1);
    gsm_at_command(fd, (char *)"at+cfun=1\r\n", (char *)"OK", NULL, 20000);
}

void substr(char * str, char * front, char * rear, char * pBuffer)
{

    int lstr ,lfront ,lrear;
    int i = 0;
    char * p = str;

    lstr = strlen(str);
    lfront = strlen(front);
    lrear = strlen(rear);

    for (p = str + lfront ; p < str +lstr -lrear ; p++)
        pBuffer[i++] = *p;
    pBuffer[i] = '\0';
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_csq(int fd)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    char intensity[8];
    int sel, len, i;
    int returnCode = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+csq\r\n", strlen("at+csq\r\n"));
    for (i = 0; i < 40; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {
            if (FD_ISSET(fd, &rfds))
            {
                /* Create '\0' for strstr,	instead of memset(buf, 0 ,sizeof(buf));   */
                buf[0] = 0;
                len = read(fd, buf, sizeof(buf) - 1);
                if(len <= 0){
                    break;
                }

                buf[len] = 0;
                if (strstr((char *)buf, "+CSQ:") != NULL)
                {
                    sscanf((char *)buf,"%*[^:]:%[^,]",intensity);
                    returnCode = atoi(intensity);
                    break;
                }
                if (strstr((char *)buf, "ERROR") != NULL) {
                    break;
                }

            }
        }
    }

    return returnCode;

}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_cesq(int fd, int * rsrq, int *rsrp)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    char prsrq[8];
    char prsrp[8];
    int sel, len, i;
    int returnCode = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+cesq\r\n", strlen("at+cesq\r\n"));
    for (i = 0; i < 40; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 0;
        timeout.tv_usec = 50000;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {
            if (FD_ISSET(fd, &rfds))
            {
                /* Create '\0' for strstr,	instead of memset(buf, 0 ,sizeof(buf));   */
                buf[0] = 0;
                len = read(fd, buf, sizeof(buf) - 1);
                if(len <= 0){
                    break;
                }

                buf[len] = 0;
                if (strstr((char *)buf, "+CESQ:") != NULL)
                {
                    sscanf((char *)buf,"%*[^:]:%*[^,],%*[^,],%*[^,],%*[^,],%[^,],%[^,]",prsrq, prsrp);
                    *rsrq = atoi(prsrq);
                    *rsrp = atoi(prsrp);
                    return 0;
                    break;
                }
                if (strstr((char *)buf, "ERROR") != NULL) {
                    break;
                }

            }
        }
    }

    return -1;

}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_iccid(int fd, char *iccid)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i, curlen;
    int returnCode = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+ccid\r\n", strlen("at+ccid\r\n"));
    curlen = 0;
    for (i = 0; i < 20; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {
            if (FD_ISSET(fd, &rfds))
            {
                len = read(fd, buf+curlen, sizeof(buf)-curlen-1);
                if(len <= 0){
                    break;
                }

                curlen += len;
                buf[curlen] = 0;
                if (strstr((char *)buf, "ERROR") != NULL) {
                    break;
                }
                if (strstr((char *)buf, "OK") != NULL)
                {
                    int j = 0;
                    char * ptr = (char *)buf;
                    for(j = 0; j< strlen((char *)buf); ++j, ++ptr)
                    {
                        if(*ptr >='0' && *ptr <= '9')
                            break;
                    }
                    sscanf((char *)ptr,"%20s",iccid);
                    returnCode = 0;
                    break;
                }

            }
        }
    }

    return returnCode;

}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int gsm_at_get_imei(int fd, char *imei)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i, curlen;
    int returnCode = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+cgsn\r\n", strlen("at+cgsn\r\n"));
    curlen = 0;
    for (i = 0; i < 20; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);

        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0)
        {
            if (FD_ISSET(fd, &rfds))
            {
                len = read(fd, buf+curlen, sizeof(buf)-curlen-1);
                if(len <= 0){
                    break;
                }

                curlen += len;
                buf[curlen] = 0;

                if (strstr((char *)buf, "ERROR") != NULL) {
                    break;
                }
                if (strstr((char *)buf, "OK") != NULL)
                {
                    int j = 0;
                    char * ptr = (char *)buf;
                    for(j = 0; j< strlen((char *)buf); ++j, ++ptr)
                    {
                        if(*ptr >='0' && *ptr <= '9')
                            break;
                    }
                    sscanf((char *)ptr,"%15s",imei);
                    
                    returnCode = 0;
                    break;
                }

            }
        }
    }

    return returnCode;

}


