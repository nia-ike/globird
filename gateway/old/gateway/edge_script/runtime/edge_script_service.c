#include "edge_script_service.h"
#include "edge_script_worker.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "../config/edge_script_cfg_load.h"
#include "../log/edge_log.h"
#include "db_node_fops.h"
#include "db_struct.h"

struct EdgeScriptService
{
    char config_base[EDGE_SCRIPT_CFG_BASE_PATH_LEN];
    char json_path[EDGE_SCRIPT_CFG_BASE_PATH_LEN + 64];
    EdgeScriptConfig cfg;
    EdgeScriptWorker *workers[EDGE_SCRIPT_MAX_SCRIPTS];
    unsigned worker_count;
    time_t json_mtime;
    int debug;
};

/*************************************************************************/
/** Function Name:       edge_script_json_mtime                            **/
/** Purpose:             获取 edge_scripts.json 文件修改时间                **/
/** Params:            path - JSON 文件路径                                **/
/** Return:            st_mtime；失败时返回 0                              **/
/** Notice:            用于热更新检测                                      **/
/*************************************************************************/
static time_t edge_script_json_mtime(const char *path)
{
    struct stat st;

    if (!path || stat(path, &st) != 0)
        return 0;
    return st.st_mtime;
}

/*************************************************************************/
/** Function Name:       edge_script_service_stop_workers                  **/
/** Purpose:             停止并回收全部脚本 worker 线程                     **/
/** Params:            svc - 服务对象                                      **/
/** Return:            None.                                               **/
/** Notice:            先 broadcast stop 再逐个 join                       **/
/*************************************************************************/
static void edge_script_service_stop_workers(EdgeScriptService *svc)
{
    unsigned i;

    if (!svc)
        return;

    for (i = 0; i < svc->worker_count; i++)
    {
        if (svc->workers[i])
            edge_script_worker_stop(svc->workers[i]);
    }
    for (i = 0; i < svc->worker_count; i++)
    {
        if (svc->workers[i])
        {
            edge_script_worker_join(svc->workers[i]);
            svc->workers[i] = NULL;
        }
    }
    svc->worker_count = 0;
}

/*************************************************************************/
/** Function Name:       edge_script_prepare_ctrl_queues                   **/
/** Purpose:             热 reload 前等待控制队列排空，超时则 flush 清队列   **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            进程不重启时共享内存队列可能残留 is_active 占用槽位   **/
/*************************************************************************/
static void edge_script_prepare_ctrl_queues(void)
{
    uint8_t ch;
    unsigned waited = 0;
    const unsigned timeout_ms = 5000U;
    const unsigned poll_ms = 20U;
    int pending;

    for (;;)
    {
        pending = 0;
        for (ch = 0; ch < DB_CFG_MAX_PORT; ch++)
        {
            if (db_ctrl_queue_pending_count(ch) > 0)
            {
                pending = 1;
                break;
            }
        }

        if (!pending || waited >= timeout_ms)
            break;

        usleep(poll_ms * 1000U);
        waited += poll_ms;
    }

    if (!pending || !DataMgmtFops.flush_command_op)
        return;

    edge_log_service_warn("ctrl queue still pending after worker stop, flushing");
    fprintf(stderr,
            "[edge_script] ctrl queue still pending after worker stop, flushing\n");
    for (ch = 0; ch < DB_CFG_MAX_PORT; ch++)
        DataMgmtFops.flush_command_op(ch);
}

/*************************************************************************/
/** Function Name:       edge_script_service_create                        **/
/** Purpose:             创建边缘脚本服务对象                               **/
/** Params:            config_base - 配置根路径；debug - 调试标志         **/
/** Return:            服务对象指针；失败返回 NULL                         **/
/** Notice:            不加载配置，需调用 reload                           **/
/*************************************************************************/
EdgeScriptService *edge_script_service_create(const char *config_base, int debug)
{
    EdgeScriptService *svc;

    svc = (EdgeScriptService *)calloc(1, sizeof(*svc));
    if (!svc)
        return NULL;

    strncpy(svc->config_base, config_base, sizeof(svc->config_base) - 1);
    snprintf(svc->json_path, sizeof(svc->json_path),
             "%s/edge_script/edge_scripts.json", config_base);
    svc->debug = debug;
    return svc;
}

/*************************************************************************/
/** Function Name:       edge_script_service_destroy                       **/
/** Purpose:             销毁服务对象并停止全部 worker                      **/
/** Params:            svc - 服务对象                                      **/
/** Return:            None.                                               **/
/** Notice:            svc 为 NULL 时直接返回                              **/
/*************************************************************************/
void edge_script_service_destroy(EdgeScriptService *svc)
{
    if (!svc)
        return;
    edge_script_service_stop_workers(svc);
    free(svc);
}

/*************************************************************************/
/** Function Name:       edge_script_service_reload                        **/
/** Purpose:             重新加载配置并为 enabled 脚本启动 worker           **/
/** Params:            svc - 服务对象                                      **/
/** Return:            0-成功（含无配置待命）；-1-配置存在但读/解析失败       **/
/** Notice:            会先停止旧 worker 再启动新 worker                   **/
/*************************************************************************/
int edge_script_service_reload(EdgeScriptService *svc)
{
    EdgeScriptConfig cfg;
    unsigned i;
    unsigned started = 0;
    time_t json_mtime;

    if (!svc)
        return -1;

    edge_script_service_stop_workers(svc);
    edge_script_prepare_ctrl_queues();
    memset(&cfg, 0, sizeof(cfg));

    if (edge_script_config_load(&cfg, svc->config_base) != 0)
    {
        edge_log_service_error("config parse failed: %s", svc->json_path);
        fprintf(stderr, "[edge_script] config parse failed: %s\n", svc->json_path);
        return -1;
    }

    json_mtime = edge_script_json_mtime(svc->json_path);
    if (json_mtime == 0)
    {
        svc->cfg = cfg;
        svc->json_mtime = 0;
        edge_log_service_info("no edge_scripts.json, idle until configured");
        if (svc->debug)
            printf("[edge_script] no edge_scripts.json, idle until configured\n");
        return 0;
    }

    edge_log_service_info("reloading edge_scripts.json");
    svc->cfg = cfg;
    svc->json_mtime = json_mtime;

    edge_log_service_info("loaded version=%s scripts=%u",
                          svc->cfg.version, svc->cfg.script_count);
    if (svc->debug)
    {
        printf("[edge_script] loaded version=%s scripts=%u\n",
               svc->cfg.version, svc->cfg.script_count);
    }

    for (i = 0; i < svc->cfg.script_count; i++)
    {
        const EdgeScriptEntry *entry = &svc->cfg.scripts[i];

        if (!entry->enabled)
            continue;

        if (access(entry->script_path, R_OK) != 0)
        {
            edge_log_service_error("script missing: %s", entry->script_path);
            fprintf(stderr, "[edge_script] script missing: %s\n", entry->script_path);
            continue;
        }

        if (svc->worker_count >= EDGE_SCRIPT_MAX_SCRIPTS)
            break;

        if (edge_script_worker_start(&svc->workers[svc->worker_count], entry, svc->debug) == 0)
        {
            edge_log_service_info("started worker id=%s file=%s",
                                  entry->id, entry->script_file);
            if (svc->debug)
                printf("[edge_script] started worker id=%s file=%s\n",
                       entry->id, entry->script_file);
            svc->worker_count++;
            started++;
        }
        else
        {
            edge_log_service_error("start worker failed: %s", entry->id);
            fprintf(stderr, "[edge_script] start worker failed: %s\n", entry->id);
        }
    }

    if (started == 0 && svc->cfg.script_count > 0)
    {
        edge_log_service_warn("no enabled script worker started");
        fprintf(stderr, "[edge_script] no enabled script worker started\n");
    }

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_script_service_poll_reload                     **/
/** Purpose:             休眠后轮询 JSON 变更并在变更时热更新               **/
/** Params:            svc - 服务对象；interval_sec - 轮询间隔（秒）       **/
/** Return:            0-无变更或 reload 成功；-1-参数无效或 reload 失败   **/
/** Notice:            interval_sec 为 0 时使用默认 3 秒                   **/
/*************************************************************************/
int edge_script_service_poll_reload(EdgeScriptService *svc, unsigned interval_sec)
{
    time_t now_mtime;

    if (!svc)
        return -1;

    if (interval_sec == 0)
        interval_sec = 3;

    sleep(interval_sec);

    now_mtime = edge_script_json_mtime(svc->json_path);
    if (now_mtime != 0 && now_mtime != svc->json_mtime)
    {
        if (svc->debug)
            printf("[edge_script] edge_scripts.json changed, reloading ...\n");
        edge_log_service_info("edge_scripts.json changed, reloading");
        return edge_script_service_reload(svc);
    }

    return 0;
}
