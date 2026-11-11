#include <syslog.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <stdlib.h>
#include <stddef.h>
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

/*************************************************************************/
/** Function Name:       gsm_at_get_cops                               **/
/** Purpose:             解析 AT+COPS? 中的运营商长名（引号内）          **/
/** Params:              oper / oper_len - 输出缓冲                     **/
/** Return:              0 成功；-1 失败                                **/
/** Notice:              例 +COPS: 0,0,"CHINA MOBILE",7                 **/
/*************************************************************************/
int gsm_at_get_cops(int fd, char *oper, size_t oper_len, int *act)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i, curlen;
    int returnCode = -1;
    char *p_cops;
    char *p_q1;
    char *p_q2;
    char *p_act;
    size_t n;

    if (!oper || oper_len == 0)
        return -1;

    oper[0] = '\0';
    if (act)
        *act = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+cops?\r\n", strlen("at+cops?\r\n"));
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
                len = read(fd, buf + curlen, sizeof(buf) - curlen - 1);
                if (len <= 0)
                    break;

                curlen += len;
                buf[curlen] = 0;

                if (strstr((char *)buf, "ERROR") != NULL)
                    break;

                if (strstr((char *)buf, "OK") != NULL)
                {
                    p_cops = strstr((char *)buf, "+COPS:");
                    if (!p_cops)
                        break;

                    returnCode = 0;
                    p_q1 = strchr(p_cops, '"');
                    if (!p_q1)
                        break;
                    p_q1++;
                    p_q2 = strchr(p_q1, '"');
                    if (!p_q2)
                        break;

                    n = (size_t)(p_q2 - p_q1);
                    if (n >= oper_len)
                        n = oper_len - 1;
                    memcpy(oper, p_q1, n);
                    oper[n] = '\0';

                    if (act) {
                        p_act = p_q2 + 1;
                        while (*p_act == ' ' || *p_act == ',' || *p_act == '\r' || *p_act == '\n')
                            p_act++;
                        if (*p_act >= '0' && *p_act <= '9')
                            *act = atoi(p_act);
                    }
                    break;
                }
            }
        }
    }

    return returnCode;
}

/*************************************************************************/
/** Function Name:       gsm_at_query_cpin                             **/
/** Purpose:             解析 AT+CPIN?，不把 OK/数字码直接当 SIM 状态    **/
/** Return:              -1 无应答；0 ready；1 pin；2 absent；3 error；4 unknown **/
/*************************************************************************/
int gsm_at_query_cpin(int fd)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i, curlen;
    int code;
    char *err;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+cpin?\r\n", strlen("at+cpin?\r\n"));
    curlen = 0;
    buf[0] = 0;
    for (i = 0; i < 20; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0 && FD_ISSET(fd, &rfds))
        {
            len = read(fd, buf + curlen, sizeof(buf) - curlen - 1);
            if (len <= 0)
                break;
            curlen += len;
            buf[curlen] = 0;

            if (strstr((char *)buf, "+CPIN:") == NULL &&
                strstr((char *)buf, "ERROR") == NULL &&
                strstr((char *)buf, "OK") == NULL)
                continue;

            if (strstr((char *)buf, "+CPIN: READY") != NULL ||
                strstr((char *)buf, "+CPIN:READY") != NULL)
                return 0;

            if (strstr((char *)buf, "NOT INSERTED") != NULL ||
                strstr((char *)buf, "not inserted") != NULL)
                return 2;

            err = strstr((char *)buf, "+CME ERROR:");
            if (err) {
                code = atoi(err + 11);
                if (code == 10)
                    return 2;
                if (code == 11 || code == 12 || code == 16 || code == 17 || code == 18)
                    return 1;
                return 3;
            }

            if (strstr((char *)buf, "SIM PIN") != NULL ||
                strstr((char *)buf, "SIM PUK") != NULL ||
                strstr((char *)buf, "PH-SIM") != NULL ||
                strstr((char *)buf, "PH-NET") != NULL)
                return 1;

            if (strstr((char *)buf, "ERROR") != NULL)
                return 3;
            if (strstr((char *)buf, "+CPIN:") != NULL || strstr((char *)buf, "OK") != NULL)
                return 4;
        }
    }

    return -1;
}

/*************************************************************************/
/** Function Name:       gsm_at_query_creg                             **/
/** Purpose:             取 AT+CREG? 的 stat，不把 1/5 折叠成布尔        **/
/** Return:              0 成功；-1 失败                                **/
/*************************************************************************/
int gsm_at_query_creg(int fd, int *stat)
{
    fd_set rfds;
    struct timeval timeout;
    unsigned char buf[BUF_SIZE];
    int sel, len, i, curlen;
    char *p;
    char *comma;

    if (!stat)
        return -1;
    *stat = -1;

    tcdrain(fd);
    tcflush(fd, TCIOFLUSH);
    write(fd, "at+creg?\r\n", strlen("at+creg?\r\n"));
    curlen = 0;
    buf[0] = 0;
    for (i = 0; i < 20; i++)
    {
        FD_ZERO(&rfds);
        FD_SET(fd, &rfds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        if ((sel = select(fd + 1, &rfds, NULL, NULL, &timeout)) > 0 && FD_ISSET(fd, &rfds))
        {
            len = read(fd, buf + curlen, sizeof(buf) - curlen - 1);
            if (len <= 0)
                break;
            curlen += len;
            buf[curlen] = 0;

            if (strstr((char *)buf, "ERROR") != NULL)
                return -1;

            if (strstr((char *)buf, "OK") == NULL && strstr((char *)buf, "+CREG:") == NULL)
                continue;

            p = strstr((char *)buf, "+CREG:");
            if (!p)
                continue;
            comma = strchr(p, ',');
            if (!comma)
                continue;
            *stat = atoi(comma + 1);
            if (*stat < 0)
                return -1;
            return 0;
        }
    }

    return -1;
}


