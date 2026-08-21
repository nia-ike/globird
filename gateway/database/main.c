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
#include "db_cfg_realtime.h"


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
	fprintf(stderr,"  -d                  : debug\n");
	fprintf(stderr,"  -h                  : Show this help message\n");
}





/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int msg_process(void *vmem)
{
    int ret;

    /* block for event */
    //ret = db_msg_event_api_pend(&msg);
    //if(ret > 0)
    {
        sleep(3);
    }
    
    return ret;
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
    struct database_ctx_t Ctx = {
        .debug = false,
        .pDB = NULL,

    };
    
    if(argc >= 2)
    {
        while((opt=getopt(argc, argv, "dh")) != -1)
        {
            switch(opt)
            {
                case 'd':
                    Ctx.debug = true;
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
        printf("please insert /dev/vmem.ko first\n");
        return -1;
    }

    Ctx.pDB = (struct db_spl_realtime_data_tbl_t *)vmem;

    db_cfg_rt_map_load(&Ctx, "/userdata/config/sample/channels/");


    while(1)
    {
        /*
            block .  msg_process()
            this function handle the message queue. for example,  key, timer, and so on ...
        */
        msg_process(vmem);
    }

    rt_database_deattach();
    return 0;
}