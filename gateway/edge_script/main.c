/*
 * 边缘脚本服务进程入口
 * 支持 -h 帮助、-d 调试、-c 配置根路径（默认 /userdata/config）
 * 加载 edge_script/edge_scripts.json，为每条 enabled 脚本启动独立 worker 线程。
 * 无 edge_script 配置目录时进入空闲待命，等待 4DH 热下发或后续补全配置。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "edge_script_config.h"
#include "config/edge_script_cfg_load.h"
#include "runtime/edge_script_service.h"
#include "log/edge_log.h"
#include "db_device.h"
#include "db_node_fops.h"

#define DEFAULT_CONFIG_BASE   "/userdata/config"
#define DEFAULT_RELOAD_SEC    3U

static int  g_debug;
static char g_config_base[EDGE_SCRIPT_CFG_BASE_PATH_LEN];

/*************************************************************************/
/** Function Name:       usage                                             **/
/** Purpose:             打印命令行帮助信息                                **/
/** Params:            prog - 程序名 argv[0]                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void usage(const char *prog)
{
    printf("Usage: %s [-h] [-d] [-c config_base] [-r reload_sec]\n", prog);
    printf("  -h             show help\n");
    printf("  -d             debug log\n");
    printf("  -c path        config root, default %s\n", DEFAULT_CONFIG_BASE);
    printf("  -r seconds     poll edge_scripts.json mtime, default %u\n",
           (unsigned)DEFAULT_RELOAD_SEC);
}

/*************************************************************************/
/** Function Name:       main                                              **/
/** Purpose:             边缘脚本服务主入口：挂接实时库并运行脚本调度       **/
/** Params:            argc - 参数个数；argv - 参数列表                   **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            默认配置根 /userdata/config，主循环轮询热更新       **/
/*************************************************************************/
int main(int argc, char **argv)
{
    EdgeScriptService *svc;
    unsigned reload_sec = DEFAULT_RELOAD_SEC;
    void *vmem;
    int opt;

    g_debug = 0;
    strncpy(g_config_base, DEFAULT_CONFIG_BASE, sizeof(g_config_base) - 1);

    while ((opt = getopt(argc, argv, "hdc:r:")) != -1)
    {
        switch (opt)
        {
            case 'h':
                usage(argv[0]);
                return 0;
            case 'd':
                g_debug = 1;
                break;
            case 'c':
                strncpy(g_config_base, optarg, sizeof(g_config_base) - 1);
                g_config_base[sizeof(g_config_base) - 1] = '\0';
                break;
            case 'r':
                reload_sec = (unsigned)atoi(optarg);
                if (reload_sec == 0)
                    reload_sec = DEFAULT_RELOAD_SEC;
                break;
            default:
                usage(argv[0]);
                return -1;
        }
    }

    if (g_debug)
        printf("[edge_script] config_base=%s reload_sec=%u\n",
               g_config_base, reload_sec);

    vmem = rt_database_attach();
    if (vmem == NULL)
    {
        fprintf(stderr, "[edge_script] rt_database_attach failed, please insert /dev/vmem.ko first\n");
        return -1;
    }
    DataMgmtFops.init(vmem);

    if (edge_log_init(g_debug) != 0)
        fprintf(stderr, "[edge_script] dev_log open failed, remote log disabled\n");

    svc = edge_script_service_create(g_config_base, g_debug);
    if (!svc)
    {
        fprintf(stderr, "[edge_script] service create failed\n");
        edge_log_shutdown();
        rt_database_deattach();
        return -1;
    }

    if (edge_script_service_reload(svc) != 0)
    {
        fprintf(stderr,
                "[edge_script] initial reload failed, will retry when config changes\n");
    }

    edge_log_service_info("edge_script_service started config_base=%s reload_sec=%u",
                          g_config_base, reload_sec);

    if (g_debug)
        printf("[edge_script] running ...\n");

    for (;;)
    {
        if (edge_script_service_poll_reload(svc, reload_sec) != 0)
        {
            if (g_debug)
                printf("[edge_script] reload failed, retry later\n");
        }
    }

    edge_script_service_destroy(svc);
    edge_log_shutdown();
    rt_database_deattach();
    return 0;
}
