#include <stdio.h>
#include <string.h> 
#include <sys/types.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <fcntl.h>   
#include <assert.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <malloc.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/ipc.h> 
#include <sys/msg.h>
#include <stdbool.h>

#include "db_cfg.h"
#include "db_device.h"
#include "db_struct.h"
#include "db_msg_process.h"
#include "db_msgevent_api.h"
#include "db_msr_msgevent_api.h"

//=======================================================================//
//= static var definition.                                                    =//
//=======================================================================//


/*
    THIS module is use for listen for keyboard and create a thread for timer.
*/



/*
    timer 
*/
struct db_list_table
{
	uint8_t		enable;
	uint32_t	timeout;
    uint32_t    reload;
};
struct db_timer_mgr
{
	struct db_list_table			timer_list[DB_TIMER_NUM_MAX];
};


/*
    this module manager struct
*/
struct db_msg_ctx_t
{
    int         keys_fd;
    pthread_t   pthread_id;
    struct db_timer_mgr db_msg_timer_mgr;
};


static struct db_msg_ctx_t db_msg_ctx;



/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           this function will block                               **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int db_msg_key_event_listen(int fd, uint32_t ms)
{

    int                     ret;
    fd_set                  rfds;
    struct timeval          tv;
    struct db_msg_evt_general_t msg_general_evt;
    struct input_event      *pkey_event = (struct input_event *)&msg_general_evt.ctx[0];
    /*
        un-block the keyboard read!, wait for 1s
    */
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);  
    /* block 1s */  
    tv.tv_sec  = ms / 1000;
    tv.tv_usec = ms % 1000 * 1000;

    /* poll the keyboard every 1s */
    ret = select(fd+1, &rfds, NULL, NULL, &tv);
    if(ret <= 0)
        return -1;
  
    ret = read(fd, pkey_event, sizeof(struct input_event));
    if(ret < 0)
        return -1;

    /* KEY event */
    if(pkey_event->type == DB_KEY_EVENT_TYPE)
    {
        switch(pkey_event->code)
        {
            case DB_KEY_EVENT_CODE_PWROFF:
            case DB_KEY_EVENT_CODE_DOWN:
            case DB_KEY_EVENT_CODE_ENTER:
                msg_general_evt.type = DB_EVENT_TYPE_KEY;
                db_msg_event_api_post(&msg_general_evt, sizeof(msg_general_evt));
            break;
        }
    }

    return 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void db_timer_event_callback(struct db_msg_ctx_t *pCtx)
{
	int i;
	struct db_timer_mgr *pMgr = &pCtx->db_msg_timer_mgr;
	
	for(i=0;i<DB_TIMER_NUM_MAX;i++)
	{
		if(pMgr->timer_list[i].enable)
		{
			if(pMgr->timer_list[i].timeout)
			{
				pMgr->timer_list[i].timeout--;
				if(pMgr->timer_list[i].timeout == 0)
				{
                    struct db_msg_evt_general_t msg_general_evt;
                    struct db_msg_timer_event_t *pTimer = (struct db_msg_timer_event_t *)&msg_general_evt.ctx;
                    
                    pMgr->timer_list[i].timeout = pMgr->timer_list[i].reload;

                    msg_general_evt.type = DB_EVENT_TYPE_TIMER;
                    pTimer->idx = i;
					db_msg_event_api_post(&msg_general_evt, sizeof(msg_general_evt));
				}
			}
		}
	}
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void db_event_timer(struct db_msg_ctx_t *pCtx)
{
    static time_t   t = 0;
    time_t          now;

    now = time(NULL);
    if(abs(now - t) >= 1)
    {
        t = now;
        db_timer_event_callback(pCtx);
    }
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void *db_msg_event_handle_thread(void *pvoid)
{
    struct db_msg_ctx_t *pCtx = (struct db_msg_ctx_t *)pvoid;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    while(1)
    {
        /* this function will block 250ms. */
        db_msg_key_event_listen(pCtx->keys_fd, DB_KEY_WAIT);

        /* timer */
        db_event_timer(pCtx);
    }

    return NULL;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_settimer(uint8_t id, uint32_t timeout)
{
	if(id >= DB_TIMER_NUM_MAX)
    {
        printf("db_msg_settimer:id > %d\n", DB_TIMER_NUM_MAX);
		return -1;
    }
	/* this timer is busy */
	if(db_msg_ctx.db_msg_timer_mgr.timer_list[id].enable)
		return -1;
    
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].enable = true;
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].timeout = timeout;
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].reload = timeout;
	return 0;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
int db_msg_destorytimer(uint8_t id)
{
	if(id >= DB_TIMER_NUM_MAX)
		return -1;
	
	/* this timer is busy */
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].enable = 0;
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].timeout = 0;
	db_msg_ctx.db_msg_timer_mgr.timer_list[id].reload = 0;
	return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
int db_msg_event_init(void)
{
    /* timer init */
    memset(&db_msg_ctx.db_msg_timer_mgr, 0, sizeof(db_msg_ctx.db_msg_timer_mgr));

    /* listen to the keyboard */
    db_msg_ctx.keys_fd = open(DB_KEY_DEV, O_RDONLY);
    if(db_msg_ctx.keys_fd < 0)
    {
        printf("DB EVENT: keyboard not ready!\n");
        return -1;
    }

    /* message queue initial */
    db_msg_event_api_init();

    /* 初始化所有通道的消息队列（动态支持1-32通道）*/
    uint8_t max_channels = db_msr_msg_event_api_get_max_channels();
    for(uint8_t i = 0; i < max_channels; i++)
    {
        if(db_msr_msg_event_api_init(i) < 0)
        {
            printf("db_msg_event_init: failed to init channel %d\n", i);
            return -1;
        }
    }

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&db_msg_ctx.pthread_id, &attr, db_msg_event_handle_thread, (void *)&db_msg_ctx) < 0)
    {
        return -1;
    }

    return 0;
}


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void db_msg_event_destory(void)
{
    int i;

    if(db_msg_ctx.keys_fd != -1)
    {
        close(db_msg_ctx.keys_fd);
        db_msg_ctx.keys_fd = -1;
    }

    for(i=0; i<DB_TIMER_NUM_MAX; i++)
    {
        db_msg_destorytimer(i);
    }
    db_msg_event_api_destory();
    db_msr_msg_event_api_destory();
}