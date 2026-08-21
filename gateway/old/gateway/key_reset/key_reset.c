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

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <linux/input.h>

#define DEV_KEY_RESET                       "/dev/input/event0"
#define DB_KEY_EVENT_TYPE                   (1)
#define DB_KEY_EVENT_CODE_RST               (12)
#define DB_KEY_EVENT_PRESSED                (1)
#define DB_KEY_EVENT_RELEASE                (0)

int main()
{
    int                     ret = 0;
    time_t                  s_time = 0;
    struct input_event      key_event;

    int keys_fd = open(DEV_KEY_RESET, O_RDONLY);
    if(keys_fd < 0)
    {
        printf("power not ready!\n");
        return -1;
    }

    while(1)
    {
        ret = read(keys_fd, &key_event, sizeof(key_event));
        /* KEY event */
        if((key_event.type == DB_KEY_EVENT_TYPE))
        {
            if(key_event.code == DB_KEY_EVENT_CODE_RST)
            {
                if (key_event.value == 1)
                {
                    s_time = time(NULL);
                }
                else 
                {
                    if((time(NULL) - s_time) >= 3)
                    {
                        system("rm -rf /userdata/config;cp -r /userdata/config.default /userdata/config; reboot -d 1;");
                        break;
                    }
                }
            }
            else
            {
                ret = 0;
            }
        }
    }
    close(keys_fd);
}