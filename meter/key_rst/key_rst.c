/*
* 描    述：独立进程管理 RST/key2（DB_KEY_EVENT_CODE_ENTER）
*           长按 10s 松开后：保存电能、关闭看门狗、复位系统
*/

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <linux/input.h>
#include <linux/watchdog.h>
#include <sys/ioctl.h>

#include "db_msgevent_api.h"

#define DEV_KEY                         "/dev/input/event0"
#define DB_KEY_EVENT_TYPE               (1)
#define DB_KEY_EVENT_CODE_ENTER         (13)    /* key2 / RST */
#define DB_KEY_EVENT_PRESSED            (1)
#define DB_KEY_EVENT_RELEASE            (0)
#define KEY_RST_LONG_PRESS_SEC          (10)
#define ENERGY_SAVE_WAIT_SEC            (2)

static void disable_watchdog(void)
{
    int fd;
    int flags;

    /* 停掉喂狗守护进程，避免随后再次打开 /dev/watchdog */
    system("killall -q watchdog 2>/dev/null");
    usleep(100000);

    fd = open("/dev/watchdog", O_WRONLY);
    if (fd < 0)
        return;

    flags = WDIOS_DISABLECARD;
    ioctl(fd, WDIOC_SETOPTIONS, &flags);

    /* magic close：写 'V' 再关闭，禁止关闭后触发复位 */
    (void)write(fd, "V", 1);
    close(fd);
}

static void key_rst_longpress_action(void)
{
    printf("key_rst: long press detected, save energy / disable watchdog / reboot\n");

    if (db_msg_event_api_init() == 0)
    {
        db_msg_event_cmd(DB_EVENT_TYPE_SAVE_ALL);
        sleep(ENERGY_SAVE_WAIT_SEC);
    }
    else
    {
        printf("key_rst: db msg init failed, skip energy save\n");
    }

    disable_watchdog();
    system("reboot -d 1 &");
}

int main(void)
{
    int                 ret;
    int                 keys_fd;
    int                 pressed = 0;
    time_t              press_time = 0;
    struct input_event  key_event;

    keys_fd = open(DEV_KEY, O_RDONLY);
    if (keys_fd < 0)
    {
        printf("key_rst: open %s failed: %s\n", DEV_KEY, strerror(errno));
        return -1;
    }

    printf("key_rst: listening %s code=%d, long-press %ds\n",
           DEV_KEY, DB_KEY_EVENT_CODE_ENTER, KEY_RST_LONG_PRESS_SEC);

    while (1)
    {
        ret = read(keys_fd, &key_event, sizeof(key_event));
        if (ret != (int)sizeof(key_event))
            continue;

        if (key_event.type != DB_KEY_EVENT_TYPE)
            continue;

        if (key_event.code != DB_KEY_EVENT_CODE_ENTER)
            continue;

        if (key_event.value == DB_KEY_EVENT_PRESSED)
        {
            pressed = 1;
            press_time = time(NULL);
        }
        else if (key_event.value == DB_KEY_EVENT_RELEASE && pressed)
        {
            pressed = 0;
            if ((time(NULL) - press_time) >= KEY_RST_LONG_PRESS_SEC)
            {
                key_rst_longpress_action();
                break;
            }
        }
    }

    close(keys_fd);
    return 0;
}
