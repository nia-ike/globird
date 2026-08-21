#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

#include "db_struct.h"
#include "db_device.h"
#include "db_node_fops.h"
#include "dev_log.h"

#include "sampling_cfg.h"
#include "config_devices.h"
#include "sampling_opc_ua_process.h"

/*********************************************************************************************************
** Function name:       usage
** Descriptions:        打印命令行帮助信息
** input parameters:    name  程序名（argv[0]）
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
static void usage(char *name)
{
    fprintf(stderr, "\nUsage: %s [options]\n", name);
    fprintf(stderr, "  -f cfg file path    : channel config json path\n");
    fprintf(stderr, "  -c channel no       : channel number\n");
    fprintf(stderr, "  -s                  : placeholder flag for init script\n");
    fprintf(stderr, "  -d                  : enable debug\n");
    fprintf(stderr, "  -h                  : show help\n");
}

/*********************************************************************************************************
** Function name:       main
** Descriptions:        OPC UA 采集进程入口：挂载实时库、加载通道配置、启动采集主循环
** input parameters:    argc  参数个数
**                      argv  参数列表（-f 通道配置 -c 通道号 -d 调试）
** output parameters:   NONE
** Returned value:      0 成功，非 0 失败
*********************************************************************************************************/
int main(int argc, char **argv)
{
    int opt;
    struct db_spl_realtime_data_tbl_t *pVmem;
    struct opc_ua_mgr_t mgr;
    struct sampling_optarg_t optarg_mgr = {
        .dump = 0,
        .channl_no = 0,
        .cfg_file = "",
    };

    memset(&mgr, 0, sizeof(mgr));

    if (argc >= 2) {
        while ((opt = getopt(argc, argv, "hdf:c:s")) != -1) {
            switch (opt) {
            case 'd':
                optarg_mgr.dump = 1;
                break;
            case 'f':
                snprintf(optarg_mgr.cfg_file, sizeof(optarg_mgr.cfg_file), "%s", optarg);
                break;
            case 'c':
                optarg_mgr.channl_no = (uint8_t)atoi(optarg);
                mgr.ChannelNo = optarg_mgr.channl_no;
                break;
            case 'h':
                usage(argv[0]);
                return 0;
            default:
                usage(argv[0]);
                return 1;
            }
        }
    }

    pVmem = rt_database_attach();
    if (pVmem == NULL) {
        printf("please insert /dev/vmem.ko first\n");
        return -1;
    }

    if (log_open(NULL) == NULL) {
        printf("please insert /dev/log.ko first\n");
        rt_database_deattach();
        return -1;
    }

    mgr.pVmem = pVmem;
    mgr.poptarg = &optarg_mgr;

    DataMgmtFops.init(pVmem);

    if (config_channel_init(&mgr, optarg_mgr.cfg_file) < 0) {
        /* 配置不存在或解析失败：报错后阻塞，避免 monit 反复拉起占用资源 */
        printf("opc_ua channel config init fail: %s, idle wait...\n",
               optarg_mgr.cfg_file[0] ? optarg_mgr.cfg_file : "(null)");
        while (1)
            sleep(3600);
    }

    opc_ua_channel_start(pVmem, &mgr);
    opc_ua_channel_process(&mgr);

    log_close();
    rt_database_deattach();
    return 0;
}
