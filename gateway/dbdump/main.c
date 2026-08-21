#include <stdio.h>
#include <string.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>

#include "db.h"
#include "db_device.h"
#include "db_node_fops.h"


// shows how to use this program
/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void usage(char *_name)
{
	fprintf(stderr,"\nUsage: %s [options] \n",_name);
	fprintf(stderr,"  -c N                 : channel number\n");
    fprintf(stderr,"  -d N                 : devices number\n");
    fprintf(stderr,"  -i N                 : node number. dump all node from device if not set this parameter\n");
    fprintf(stderr,"  -w N.M               : write mode if set\n");
    fprintf(stderr,"  -r                   : print ctrl info\n");
	fprintf(stderr,"  -h                   : Show this help message\n");
}







/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int main(int argc, char **argv)
{
    void    *vmem;
    int     ret;
    int     opt;
    int     write = false;
    int     ctrl = 0;
    uint32_t chnlNo = 0, devNo = 0;
    int     indexNo = -1;
    double  lfval;
    int64_t i64val;
    struct db_spl_realtime_data_tbl_t *pDB;
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
    
    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "hc:d:i:w:r:")) != -1)
        {
            switch(opt)
            {
                case 'c':
                    chnlNo = atoi(optarg);
                break;
                case 'd':
                    devNo = atoi(optarg);
                break;
                case 'i':
                    indexNo = atoi(optarg);
                break;
                case 'w':
                    write = true;
                    lfval = atof(optarg);
                break;
                case 'r':
                    ctrl = atoi(optarg);
                break;
                case 'h':
                    usage(argv[0]);
                    exit(0);
                break;
                
            }
        }
    }
    

    /*
        database attach:
            it just get the vmem pointer from /dev/vmem  (mmap)
    */
    vmem = rt_database_attach();
    if(vmem == NULL)
    {
        printf("please insert /dev/nodetable.ko first\n");
        return -1;
    }

    pDB = (struct db_spl_realtime_data_tbl_t *)vmem;
    
    DataMgmtFops.init(pDB);

    if(ctrl)
    {
        int j;
        struct db_spl_channel_ctr_t       *pctrl;

        ctrl = ctrl > 64 ? 63 : ctrl;

        printf("\n--------------------\n");
        for(j=0;j<DB_CFG_MAX_CTRL_QUEUE;j++)
        {
            printf("chnl:%d  checked:%d   function:%d  dblink:0x%X   value:%lf\n", ctrl, pDB->ctrl[ctrl][j].is_active, pDB->ctrl[ctrl][j].funcode, pDB->ctrl[ctrl][j].db_linkid, pDB->ctrl[ctrl][j].value);
        }
        printf("--- ctrl_extend_region (overflow) ---\n");
        for(j=0;j<DB_CFG_CTRL_EXTEND_SLOTS;j++)
        {
            printf("chnl:%d ext[%d] checked:%d   function:%d  dblink:0x%X   value:%lf\n", ctrl, j,
                pDB->ctrl_extend_region.extend.context[ctrl][j].is_active, pDB->ctrl_extend_region.extend.context[ctrl][j].funcode,
                pDB->ctrl_extend_region.extend.context[ctrl][j].db_linkid, pDB->ctrl_extend_region.extend.context[ctrl][j].value);
        }

    }
    else 
    {
        if(write == false)
        {
            if(indexNo < 0)
            {
                struct db_spl_remap_info_t *pInfo;
                /* 不指定点号，即当前设备所有点读取 */
                pInfo = DataMgmtFops.getIndexByPortAndDev(chnlNo, devNo);
                if(!pInfo)
                {
                    printf("WARN:Can not find NODE:%X\n", DB_CONVENT_CHNLDEV_GROUP(chnlNo, devNo));
                    goto __err;
                }

                printf("Dump all node from %04X  NodeNum:%04d\n", DB_CONVENT_CHNLDEV_GROUP(chnlNo, devNo), pInfo->node_num);
                printf("------------------------------------\n");
                for(int i=0; i<pInfo->node_num; i++)
                {
                    if(chnlNo == 193)
                    {
                        ret = DataMgmtFops.readpoint_int64(chnlNo, devNo, i, &i64val);
                        printf("U%02X%02X%XXX%02X => %lld\n", chnlNo, devNo, (i >> 12) & 0xf, i & 0xff, i64val);
                        
                    }
                        
                    else 
                    {
                        ret = DataMgmtFops.readpoint(chnlNo, devNo, i, &lfval);
                        printf("%02X%02X%XXX%02X => %lf\n", chnlNo, devNo, (i >> 12) & 0xf, i & 0xff, lfval); 
                    }
                        
                }
            }
            else
            {
                /* 指定点号读取 */
                if(chnlNo == 193)
                {
                    ret = DataMgmtFops.readpoint_int64(chnlNo, devNo, indexNo, &i64val);
                    if(ret < 0)
                    {
                        printf("WARN:Can not find NODE\n");
                        goto __err;
                    }
                    printf("%02X%02X%XXX%02X => %lld\n", chnlNo, devNo, (indexNo >> 12) & 0xf, indexNo & 0xff, i64val); 
                }    
                else 
                {
                    ret = DataMgmtFops.readpoint(chnlNo, devNo, indexNo, &lfval);
                    if(ret < 0)
                    {
                        printf("WARN:Can not find NODE\n");
                        goto __err;
                    }
                    printf("%02X%02X%XXX%02X => %lf\n", chnlNo, devNo, (indexNo >> 12) & 0xf, indexNo & 0xff, lfval); 
                }
            }    
        }
        else 
        {
            if(indexNo < 0)
                indexNo = 0;

            ret = DataMgmtFops.writepoint(chnlNo, devNo, indexNo, &lfval);
            if(ret < 0)
            {
                        printf("WARN:Can not find NODE\n");
                goto __err;
            }

            printf("%02X%02X%XXX%02X => %lf\n", chnlNo, devNo, (indexNo >> 12) & 0xf, indexNo & 0xff, lfval); 

        }
    }
     
__err:
    rt_database_deattach();
    return 0;
}