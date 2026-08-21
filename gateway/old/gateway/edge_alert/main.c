/*
* Copyright (c) 2024
* All rights reserved.
*
* 文件名称：main.c
* 文件标识：
* 描    述：edge_alert 告警服务进程入口
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：创建
* 完成日期：20240702
*
*/

#include "edge_alert_service.h"
#include "edge_alert_log.h"

#include "db_node_fops.h"
#include "db_device.h"
#include "db_struct.h"

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

/* 常量定义 */
#define EDGE_ALERT_DEFAULT_CONFIG_BASE    "/userdata/config"
#define EDGE_ALERT_PID_FILE               "/var/run/edge_alert.pid"
#define EDGE_ALERT_VERSION                "1.0.0"

/* 全局变量 */
static struct EdgeAlertServiceContext *g_service_ctx = NULL;
static volatile sig_atomic_t g_running = 1;
static volatile sig_atomic_t g_reload_signal = 0;

/*************************************************************************/
/** Function Name:       edge_alert_print_usage                              **/
/** Purpose:             打印命令行使用说明                                **/
/*************************************************************************/
static void edge_alert_print_usage(const char *prog_name)
{
    printf("Usage: %s [options]\n", prog_name);
    printf("Options:\n");
    printf("  -c <path>   Configuration base path (default: %s)\n", EDGE_ALERT_DEFAULT_CONFIG_BASE);
    printf("  -b          Run in background\n");
    printf("  -d 0|1      Debug log to stderr (default: 0)\n");
    printf("  -h          Show this help message\n");
    printf("  -v          Show version\n");
}

/*************************************************************************/
/** Function Name:       edge_alert_print_version                            **/
/** Purpose:             打印版本信息                                    **/
/*************************************************************************/
static void edge_alert_print_version(void)
{
    printf("edge_alert version %s\n", EDGE_ALERT_VERSION);
    printf("Copyright (c) 2024\n");
}

/*************************************************************************/
/** Function Name:       edge_alert_write_pid_file                            **/
/** Purpose:             写入PID文件                                     **/
/*************************************************************************/
static int edge_alert_write_pid_file(const char *pid_file)
{
    FILE *fp;

    fp = fopen(pid_file, "w");
    if (!fp)
    {
        edge_log(LOG_ERROR, "Failed to write PID file: %s", pid_file);
        return -1;
    }

    fprintf(fp, "%d\n", getpid());
    fclose(fp);

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_alert_remove_pid_file                          **/
/** Purpose:             删除PID文件                                     **/
/*************************************************************************/
static void edge_alert_remove_pid_file(const char *pid_file)
{
    remove(pid_file);
}

/*************************************************************************/
/** Function Name:       edge_alert_signal_handler                           **/
/** Purpose:             信号处理函数                                  **/
/*************************************************************************/
static void edge_alert_signal_handler(int sig)
{
    switch (sig)
    {
    case SIGINT:
    case SIGTERM:
        /* 终止信号 */
        edge_log(LOG_INFO, "Received signal %d, stopping service...", sig);
        g_running = 0;
        if (g_service_ctx)
        {
            edge_alert_service_stop(g_service_ctx);
        }
        break;

    case SIGHUP:
        /* 重新加载配置信号 */
        edge_log(LOG_INFO, "Received SIGHUP, reloading configuration...");
        g_reload_signal = 1;
        break;

    case SIGUSR1:
        /* 自定义信号 - 可用于打印统计信息 */
        edge_log(LOG_INFO, "Received SIGUSR1");
        break;

    default:
        break;
    }
}

/*************************************************************************/
/** Function Name:       edge_alert_setup_signals                            **/
/** Purpose:             设置信号处理                                  **/
/*************************************************************************/
static void edge_alert_setup_signals(void)
{
    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = edge_alert_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;

    /* 注册信号处理 */
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);

    /* 忽略SIGPIPE */
    signal(SIGPIPE, SIG_IGN);
}

/*************************************************************************/
/** Function Name:       edge_alert_daemonize                                **/
/** Purpose:             转为守护进程                                    **/
/*************************************************************************/
static int edge_alert_daemonize(void)
{
    pid_t pid;

    /* 第一次fork */
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }
    if (pid > 0)
    {
        /* 父进程退出 */
        exit(0);
    }

    /* 创建新会话 */
    if (setsid() < 0)
    {
        perror("setsid");
        return -1;
    }

    /* 第二次fork */
    pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return -1;
    }
    if (pid > 0)
    {
        /* 第一个子进程退出 */
        exit(0);
    }

    /* 更改工作目录 */
    chdir("/");

    /* 重定向标准文件描述符 */
    freopen("/dev/null", "r", stdin);
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);

    return 0;
}

/*************************************************************************/
/** Function Name:       main                                                **/
/** Purpose:             进程入口                                          **/
/*************************************************************************/
int main(int argc, char *argv[])
{
    const char *config_base = EDGE_ALERT_DEFAULT_CONFIG_BASE;
    int background_mode = 0;
    int debug_mode = 0;
    int opt;
    int ret;
    void *vmem = NULL;

    /* 解析命令行参数 */
    while ((opt = getopt(argc, argv, "bc:d:hv")) != -1)
    {
        switch (opt)
        {
        case 'c':
            config_base = optarg;
            break;

        case 'b':
            background_mode = 1;
            break;

        case 'd':
            debug_mode = (optarg && atoi(optarg) != 0) ? 1 : 0;
            break;

        case 'h':
            edge_alert_print_usage(argv[0]);
            return 0;

        case 'v':
            edge_alert_print_version();
            return 0;

        default:
            edge_alert_print_usage(argv[0]);
            return 1;
        }
    }

    /* 打印启动信息（仅 debug 模式） */
    if (debug_mode)
    {
        edge_alert_print_version();
        printf("Configuration base: %s\n", config_base);
        printf("Background mode: %s\n", background_mode ? "yes" : "no");
    }

    /* 转为守护进程 */
    if (background_mode)
    {
        if (edge_alert_daemonize() != 0)
        {
            fprintf(stderr, "Failed to daemonize\n");
            return 1;
        }
    }

    /* 写入PID文件 */
    if (edge_alert_write_pid_file(EDGE_ALERT_PID_FILE) != 0)
    {
        fprintf(stderr, "Failed to create PID file\n");
        return 1;
    }

    /* 设置信号处理 */
    edge_alert_setup_signals();

    /* 附加实时数据库共享内存 */
    vmem = rt_database_attach();
    if (vmem == NULL)
    {
        fprintf(stderr, "[edge_alert] rt_database_attach failed, please insert /dev/vmem.ko first\n");
        edge_alert_remove_pid_file(EDGE_ALERT_PID_FILE);
        return 1;
    }

    /* 初始化数据库操作接口 */
    DataMgmtFops.init(vmem);

    /* 初始化日志 */
    edge_alert_log_init(debug_mode);
    edge_log(LOG_INFO, "Starting edge_alert service...");

    /* 创建服务上下文 */
    g_service_ctx = edge_alert_service_create(config_base);
    if (!g_service_ctx)
    {
        /* 配置/初始化失败：报错后阻塞，避免 monit 反复拉起占用资源 */
        edge_log(LOG_ERROR, "Failed to create service context, idle wait...");
        while (g_running)
            sleep(3600);
        edge_alert_remove_pid_file(EDGE_ALERT_PID_FILE);
        rt_database_deattach();
        return 1;
    }

    edge_log(LOG_INFO, "Service initialized, entering main loop");

    /* 主循环 */
    while (g_running)
    {
        /* 检查重载信号 */
        if (g_reload_signal)
        {
            g_reload_signal = 0;
            edge_alert_service_reload_config(g_service_ctx, config_base);
        }

        /* 执行一次评估周期 */
        ret = edge_alert_service_poll(g_service_ctx);
        if (ret != 0)
        {
            edge_log(LOG_ERROR, "Service poll failed: %d", ret);
            /* 继续运行，不退出 */
        }

        /* 按配置的 poll_interval_ms 休眠，等待下一评估周期 */
        edge_alert_service_wait_next_poll(g_service_ctx);
    }

    /* 清理 */
    edge_log(LOG_INFO, "Shutting down service...");
    edge_alert_service_destroy(g_service_ctx);
    g_service_ctx = NULL;

    /* 删除PID文件 */
    edge_alert_remove_pid_file(EDGE_ALERT_PID_FILE);

    edge_log(LOG_INFO, "Service stopped");
    edge_alert_log_shutdown();

    /* 分离实时数据库共享内存 */
    rt_database_deattach();

    return 0;
}
