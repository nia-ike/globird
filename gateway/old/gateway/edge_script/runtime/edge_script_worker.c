#include "edge_script_worker.h"

#include "../lua/edge_lua_vm.h"
#include "../log/edge_log.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct EdgeScriptWorker
{
    pthread_t thread;
    EdgeScriptEntry entry;
    EdgeLuaVm vm;
    int debug;
    volatile int stop;
    int started;
};

/*************************************************************************/
/** Function Name:       edge_script_worker_thread                         **/
/** Purpose:             单脚本 worker 线程：加载 Lua 并调用 run(ctx)       **/
/** Params:            arg - EdgeScriptWorker 指针                         **/
/** Return:            NULL                                                  **/
/** Notice:            线程结束时自动关闭 VM                               **/
/*************************************************************************/
static void *edge_script_worker_thread(void *arg)
{
    EdgeScriptWorker *worker = (EdgeScriptWorker *)arg;

    if (edge_lua_vm_open(&worker->vm, worker->entry.id, worker->debug) != 0)
    {
        edge_log_script_error(worker->entry.id, "vm open failed");
        fprintf(stderr, "[edge_script] worker %s: vm open failed\n", worker->entry.id);
        return NULL;
    }

    if (edge_lua_vm_load_file(&worker->vm, worker->entry.script_path) != 0)
    {
        edge_lua_vm_close(&worker->vm);
        return NULL;
    }

    worker->vm.worker.stop = 0;
    worker->stop = 0;
    if (edge_lua_vm_call_run(&worker->vm) != 0)
    {
        if (!worker->stop)
        {
            edge_log_script_error(worker->entry.id, "run(ctx) failed");
            fprintf(stderr, "[edge_script] worker %s: run(ctx) failed\n", worker->entry.id);
        }
    }

    edge_log_service_info("worker stopped id=%s", worker->entry.id);

    edge_lua_vm_close(&worker->vm);
    return NULL;
}

/*************************************************************************/
/** Function Name:       edge_script_worker_start                          **/
/** Purpose:             为单条脚本创建并启动 worker 线程                   **/
/** Params:            out_worker - 输出 worker 指针；entry - 脚本配置；   **/
/**                      debug - 调试标志                                  **/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            成功时 out_worker 由 join 侧释放                      **/
/*************************************************************************/
int edge_script_worker_start(EdgeScriptWorker **out_worker, const EdgeScriptEntry *entry, int debug)
{
    EdgeScriptWorker *worker;
    int rc;

    if (!out_worker || !entry)
        return -1;

    worker = (EdgeScriptWorker *)calloc(1, sizeof(*worker));
    if (!worker)
        return -1;

    worker->entry = *entry;
    worker->debug = debug;
    worker->stop = 0;

    rc = pthread_create(&worker->thread, NULL, edge_script_worker_thread, worker);
    if (rc != 0)
    {
        free(worker);
        return -1;
    }

    worker->started = 1;
    *out_worker = worker;
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_script_worker_stop                           **/
/** Purpose:             请求 worker 停止（置 stop 标志）                   **/
/** Params:            worker - worker 指针                                **/
/** Return:            None.                                               **/
/** Notice:            需配合 join 等待线程退出                            **/
/*************************************************************************/
void edge_script_worker_stop(EdgeScriptWorker *worker)
{
    if (!worker)
        return;
    worker->stop = 1;
    worker->vm.worker.stop = 1;
}

/*************************************************************************/
/** Function Name:       edge_script_worker_join                           **/
/** Purpose:             等待 worker 线程结束并释放 worker 对象             **/
/** Params:            worker - worker 指针                                **/
/** Return:            None.                                               **/
/** Notice:            未启动的 worker 直接返回                            **/
/*************************************************************************/
void edge_script_worker_join(EdgeScriptWorker *worker)
{
    if (!worker || !worker->started)
        return;
    pthread_join(worker->thread, NULL);
    worker->started = 0;
    free(worker);
}
