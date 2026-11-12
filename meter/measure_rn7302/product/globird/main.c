/**
 * @file        main.c
 * @brief       主函数
 *
 * Copyright (c) 2026, 广东碳索智控科技有限公司. All rights reserved.
 *
 * @par 修订日志
 *  日期        修订人    版本    修订内容
 * --------------------------------------------------------------
 *  2026-10-28  yansh    1.0.0   主函数
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <pthread.h>
#include "port_database.h"
#include "port_rn7302.h"
#include "db_msr_msgevent_api.h"
#include "db_msgevent_api.h"
#include "db_device.h"
#include "algorithm_fullwave.h"
#include "algorithm_fundwave.h"
#include "algorithm_halfwave.h"
#include "msr_config.h"
#include "msr_data.h"
#include "msr_input.h"
#include "rn7302.h"

struct measure_ctx
{
    int                             test;
    int                             debug;
    int32_t                         refresh;
    int                             chip;

    int                             fd_device;
    /*
        总数据和参数集合(驱动形态的 共享内存)
    */
    struct db_database_type         *pVmem;
};


/**
 * @brief       使用说明
 * @param[in]    _name    程序名称
 */
static void usage(char *_name)
{
    fprintf(stderr,"\nUsage: %s [options] \n",_name);
    fprintf(stderr,"  -d                  : debug. print message \n");
    fprintf(stderr,"  -t                  : test.  \n");
    fprintf(stderr,"  -c                  : choose ENERGY CHIP. 0-chip1(grid)  1-chip2(hybird) 2-chip3(pv)\n");
    fprintf(stderr,"  -r                  : refresh time  ms,for example:10.  \n");
    fprintf(stderr,"  -n N                : set max message channels (default: 2, must match db_process -c N)\n");
    fprintf(stderr,"  -h                  : Show this help message\n");
}

/**
 * @brief       RMS定时器线程
 * @param[in]   param    上下文
 * @return      NULL
 */
void *RMSTimerThread(void *param)
{
    struct measure_ctx *pctx = (struct measure_ctx *)param;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    /* Attempt to open non blocking the access dev */
    while (1)
    {
        db_msr_msg_event_cmd(pctx->chip, DB_MSR_EVENT_TYPE_TIMER);
        usleep(pctx->refresh * 1000);
    }

    return NULL;
}

/**
 * @brief       初始化测量
 * @param[in]    pCtx    上下文
 * @return      0 成功；-1 失败
 */
int32_t msr_init(struct measure_ctx *pCtx)
{
    struct msr_config cfg = {0};
    struct db_database_measure_base_type *pMeasure;

    if (pCtx == NULL || pCtx->pVmem == NULL) {
        return -1;
    }

    pMeasure = &(pCtx->pVmem->db_measurebase[pCtx->chip]);

    port_db_copy_config_to_measure(&cfg, pMeasure);
    rn7302_init(&cfg);

    pthread_attr_t attr;
    pthread_t m_hTimerThread;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (!pthread_create(&m_hTimerThread, &attr, RMSTimerThread, pCtx) == 0)
    {
        m_hTimerThread = 0;
        printf("Create Timer Thread failed.\n");
    }

    return 0;
}

/**
 * @brief       测量处理
 * @param[in]    pCtx    上下文
 * @return      0 成功；-1 失败
 */
static int msr_process(struct measure_ctx *pCtx)
{
#define HALFWAVE_NUM 2

    static struct msr_config cfg  = {0};
    static struct msr_data   data = {0};
    static struct msr_input  in   = {0};
    static uint32_t refreshNum = 1;
    static uint16_t hw = 0;
    struct db_database_measure_base_type *pMeasure;

    if (pCtx == NULL || pCtx->pVmem == NULL) {
        return -1;
    }

    pMeasure = &(pCtx->pVmem->db_measurebase[pCtx->chip]);

    refreshNum = (1000 / pCtx->refresh);
    refreshNum = refreshNum == 0 ? 1 : refreshNum;
    hw++;

    /* 每次计算前，先更新配置 */
    port_db_copy_config_to_measure(&cfg, pMeasure);

    /* 10ms计算一次半波 */
    if (0 != algorithm_halfwave(&data, &cfg, &in)) {
        return -1;
    }

    /* 1s计算一次全波和基波 */
    if ((hw % refreshNum) == 0) {
        hw = 0;
        if ((0 != algorithm_fullwave(&data, &cfg, &in))
        ||  (0 != algorithm_fundwave(&data, &cfg, &in))) {
            return -1;
        }
    }

    /* 将计算结果写入数据库 */
    port_db_copy_data_to_database(&data, pMeasure);

    return 0;
}

/**
 * @brief       测量线程处理
 * @param[in]    pCtx    上下文
 * @return      0 成功；-1 失败
 */
void *msr_thread_proc(struct measure_ctx *pCtx)
{
    struct db_msr_msg_evt_general_t msg;

    while (1)
    {
        db_msr_msg_event_api_pend(pCtx->chip, &msg);
        switch (msg.type)
        {
        case DB_MSR_EVENT_TYPE_TIMER:
            msr_process(pCtx);
            break;
        default:
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int     ret;
    void    *pVmem;
    int     opt;
    uint8_t max_msg_channels = 2;  // 默认2个通道，需与 db_process 保持一致

    struct measure_ctx ctx = {
        .test = 0,
        .debug = 0,
        .refresh = 10,
        .chip = 0,
        .fd_device = -1,
    };

    if (argc >= 2)
    {
        while((opt=getopt(argc, argv, "c:d:t:r:n:h")) != -1)
        {
            switch(opt)
            {
                case 'd':
                    ctx.debug = atoi(optarg);
                break;
                case 't':
                    ctx.test = atoi(optarg);
                break;
                case 'r':
                    ctx.refresh = atoi(optarg);;
                break;
                case 'c':
                    ctx.chip = atoi(optarg);
                break;
                case 'n':
                    max_msg_channels = (uint8_t)atoi(optarg);
                    if(max_msg_channels < 1 || max_msg_channels > 32) {
                        fprintf(stderr, "Error: max_channels must be between 1 and 32\n");
                        exit(1);
                    }
                    printf("Set max message channels to: %d\n", max_msg_channels);
                break;
                case 'h':
                {
                    usage(argv[0]);
                    exit(0);
                    break;
                }
                default:
                    break;
            }
        }
    }



    // 初始化数据库
    pVmem = rt_database_attach();
    if(pVmem == NULL)
        goto __database_err;
    ctx.pVmem = pVmem;

    /*
     * 重要：必须在 msr_init 之前配置消息通道！
     * 因为 msr_init 会创建 RMSTimerThread 线程，
     * 该线程会立即调用 db_msr_msg_event_cmd 发送消息
     */
    ret = db_msg_event_api_init();
    if(ret < 0)
        goto __db_msg_event_api_init_err;

    /* 配置消息通道数（必须在 msr_init 之前调用） */
    printf("Configuring message API with %d channels\n", max_msg_channels);
    ret = db_msr_msg_event_api_config(max_msg_channels);
    if(ret < 0)
    {
        printf("Failed to configure message channels\n");
        goto __db_msg_event_api_init_err;
    }

    ret = db_msr_msg_event_api_init(ctx.chip);
    if(ret < 0)
        goto __db_msr_msg_event_api_init_err;

    // 打开rn7302设备文件
    ret = MeterChipInit(ctx.chip);
    if(ret < 0)
        goto __msr_init_err;
    // 保存文件描述符
    ctx.fd_device = ret;

    /*
        msr processer. this is a loop
    */
   ret = msr_init(&ctx);
    if(ret < 0)
        goto __msr_init_err;

    msr_thread_proc(&ctx);

    /*
        before exit, release all the resource.
    */
__database_err:
    rt_database_deattach();

__msr_init_err:
    MeterChipDestory();

__db_msg_event_api_init_err:
    db_msg_event_api_destory();

__db_msr_msg_event_api_init_err:
    db_msr_msg_event_api_destory();

    return 0;
}
