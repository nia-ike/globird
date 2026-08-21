/*
 * File      : config_devices.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */


#include <stdint.h>
#include <pthread.h>
#include <sys/time.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <dlfcn.h>    //

#include "pt.h"
#include "sampling_cfg.h"
#include "sampling_channelpoll.h"
#include "sampling_bypass.h"
#include "sampling_com_baseclass.h"
#include "db_node_fops.h"
#include "debug.h"
#include "db_struct.h"
#include "db_device.h"

#include "dev_log.h"

#include <unistd.h>
#include "./lib/crc16.h"


static int32_t ChnnlPoll_PTCoroutine_Poll(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr);
static int32_t ChnnlPoll_PTCoroutine_Ctrl(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr);
static int32_t ChnnlPoll_PTCoroutine_Trans(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr);


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
void *ChnnlPoll_thread_entry(void* parameter)
{
	struct pt                                   pt_poll;
	struct pt                                   pt_ctrl;
	struct pt                                   pt_trans;
	struct channelpoll_mgr_t 					*pChnlPollMgr; 	

	PT_INIT(&pt_poll);
	PT_INIT(&pt_ctrl);
	PT_INIT(&pt_trans);

	pChnlPollMgr = (struct channelpoll_mgr_t *)parameter;
	/* init base communication class */
	pChnlPollMgr->pBaseComm->init(&pChnlPollMgr->com_ctx);
	/*
		all frame no respond for X times >  N times, we will disconnect phy and re-connect phy
		this is the counter for No respond
	*/
	pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;

	pChnlPollMgr->pBaseComm->connect(&pChnlPollMgr->com_ctx);

	/* create */
	//cfg_ReqpktInit(pChnlPollMgr->ChannelNo);
	while(1)
	{
		/* 临时调试透传：命中则阻塞旁路，结束后回 poll */
		if (ChnnlPoll_BypassTryRun(pChnlPollMgr))
			continue;

		/* poll */
		ChnnlPoll_PTCoroutine_Poll(&pt_poll, pChnlPollMgr);
		/* transpant */
		ChnnlPoll_PTCoroutine_Trans(&pt_trans, pChnlPollMgr);
		/* control */
		ChnnlPoll_PTCoroutine_Ctrl(&pt_ctrl, pChnlPollMgr);
		
		usleep(5000);
	}

	//cfg_ReqpktDeInit(pChnlPollMgr->ChannelNo);

	
	pChnlPollMgr->pBaseComm->disconnect(&pChnlPollMgr->com_ctx);

	return NULL;
}




/************************************************************************
ChnnlPoll_CommQueryData(struct channelpoll_mgr_t *pChnlPollMgr)
输入参数�?
输出参数�?
返回�? �?
说明    �?
************************************************************************/
static int32_t ChnnlPoll_CommQueryData(struct channelpoll_mgr_t *pChnlPollMgr, struct config_devices_info_attr *pDevInfoAttr)
{
	int32_t							    nread;

    if(pChnlPollMgr->pBaseComm == NULL)
	{
        return -1;
	}

    /* 清空接收缓冲�?*/
    pChnlPollMgr->pBaseComm->flush(&pChnlPollMgr->com_ctx);

	if(pChnlPollMgr->pt_pollmgr.pCurRequestlist->pfnBeforeSendCallback)
		pChnlPollMgr->pt_pollmgr.pCurRequestlist->pfnBeforeSendCallback(pDevInfoAttr, pChnlPollMgr->pt_pollmgr.pCurRequestlist);

	pChnlPollMgr->com_ctx.channel_no = pChnlPollMgr->ChannelNo;
	pChnlPollMgr->com_ctx.dev_no     = pDevInfoAttr->logic_id;
    pChnlPollMgr->pBaseComm->write(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_pollmgr.pCurRequestlist->pxmit_buff, pChnlPollMgr->pt_pollmgr.pCurRequestlist->xmit_len);

	// log - write
//	log_write(pChnlPollMgr->ChannelNo, pDevInfoAttr->logic_id, LOG_TX, pChnlPollMgr->pt_pollmgr.pCurRequestlist->pxmit_buff,pChnlPollMgr->pt_pollmgr.pCurRequestlist->xmit_len);

    //如果需要回复的报文 /* 设置接收超时时间(配置以ms为单�? */
	pChnlPollMgr->com_ctx.channel_no = pChnlPollMgr->ChannelNo;
	pChnlPollMgr->com_ctx.dev_no     = pDevInfoAttr->logic_id;

    nread = pChnlPollMgr->pBaseComm->read(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_pollmgr.recv_buff, FRAME_RECVBUF_SIZE, pChnlPollMgr->pChannelAttr->working_attr.RecvTimeOut);
	if(nread > 0)
    {       
        /* 数据帧处理回调函�?*/
        int32_t ret ;

		/* 装载接收缓冲区到 协议模块中 */
		pChnlPollMgr->pt_pollmgr.pCurRequestlist->precv_buff = pChnlPollMgr->pt_pollmgr.recv_buff;
		pChnlPollMgr->pt_pollmgr.pCurRequestlist->recv_len = nread;
		if(!pChnlPollMgr->pt_pollmgr.pCurRequestlist->pfnReceiveCallback)
			return 0;

        ret = pChnlPollMgr->pt_pollmgr.pCurRequestlist->pfnReceiveCallback(pDevInfoAttr, pChnlPollMgr->pt_pollmgr.pCurRequestlist);
        if(ret != 0)
        {
            return -1;
        }
        else
        {
            return 0;
        }
    }
	else if(nread == 0)
    {
		return -2;
	}
	else 
	{
		return -3;
	}
}





/************************************************************************
函数功能：协程�?轮询数据的协�? 注意函数内部不允许使用switch
           PTCoroutine_Poll   轮询所有设备的所有数据帧�?
输入参数�?
输出参数�?
返回�? �?
说明    �?
************************************************************************/
static int32_t ChnnlPoll_PTCoroutine_Poll(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr)
{
	double				fVal;
	int					ret;
	
	/* A protothread function must begin with PT_BEGIN() which takes a
	     pointer to a struct pt. */
	PT_BEGIN(pt);

	while(1)	
	{
		/* 轮训间隔时间 */
		timer_set(&pChnlPollMgr->pt_pollmgr.InvPollTimer, pChnlPollMgr->pChannelAttr->working_attr.inv_time);
		PT_WAIT_UNTIL(pt, timer_expired(&pChnlPollMgr->pt_pollmgr.InvPollTimer));

		/*
			sometime maybe haven't setting the devices. this will poll continusly forever.
			it will cost must more CPU. so if devices is empty. we must usleep()
		*/
		pChnlPollMgr->pt_pollmgr.is_empty = true;

		    /* We loop forever here. */
		    /*
		                遍历
		                -------    -------
		                | dev | -> | dev |
		                -------    -------
		                    |
		                    |-> -------- -> --------
		                        | frame|    | frame|
		                        --------    --------
		                            |
		                            |-> --------  -> --------
		                                | m_PollMgr.Repeat|    | m_PollMgr.Repeat|
		                                --------     --------

	    */
		/*
			go through the SUPPORT device ID[1-8], and findout exist devices, and get the request message list head

						遍历
		                -------    -------
		                | dev | -> | dev |
		                -------    -------
		*/
		slist_for_each_entry(pChnlPollMgr->pt_pollmgr.pDevInfoAttr, &pChnlPollMgr->pChannelAttr->devices_list_head.slist, slist)
		{
			if(!pChnlPollMgr->pt_pollmgr.pDevInfoAttr)
			{
				PT_YIELD(pt);
			}

			/* clear the error counter */
			pChnlPollMgr->pt_pollmgr.icomErr 	= 0;
			pChnlPollMgr->pt_pollmgr.icomTot 	= 0;
			pChnlPollMgr->pt_pollmgr.icomNoRec 	= 0;


			/*
				查询该设备，一问一答帧是否为空，
				如果为空，尝试调用 该协议内部的报文流程

				比如用于IEC103等 需要时序的流程，由驱动自身进行发送接受处理
			*/
			if(slist_isempty(&pChnlPollMgr->pt_pollmgr.pDevInfoAttr->pkt_request_list_head.slist))
			{
				if(pChnlPollMgr->pt_pollmgr.pDevInfoAttr->pfnFlowCallback)
				{
					ret = pChnlPollMgr->pt_pollmgr.pDevInfoAttr->pfnFlowCallback(pChnlPollMgr->pt_pollmgr.pDevInfoAttr, pChnlPollMgr->pBaseComm, &pChnlPollMgr->com_ctx);
					if(ret == 0)
						pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;
				}
				else 
				{
					fVal = 1;
					DataMgmtFops.writepoint(pChnlPollMgr->ChannelNo, pChnlPollMgr->pt_pollmgr.pDevInfoAttr->logic_id, 0, &fVal);
					usleep(5000);
					PT_YIELD(pt);
				}
			}
			else 
			{
				/*
							|
		                    |-> -------- -> --------
		                        | frame|    | frame|
		                        --------    --------
				*/

                if(slist_isempty(&pChnlPollMgr->pt_pollmgr.pDevInfoAttr->pkt_request_list_head.slist))
                {
                    usleep(5000);
					PT_YIELD(pt);
                }
                else 
                {
                    slist_for_each_entry(pChnlPollMgr->pt_pollmgr.pCurRequestlist, &pChnlPollMgr->pt_pollmgr.pDevInfoAttr->pkt_request_list_head.slist, slist)
                    {
                        /* found frame, this poll is not empty */
                        pChnlPollMgr->pt_pollmgr.is_empty = false;

                        pChnlPollMgr->pt_pollmgr.icomTot++;
                        /* 重发*/			
                        for(pChnlPollMgr->pt_pollmgr.repeat=0; pChnlPollMgr->pt_pollmgr.repeat<pChnlPollMgr->pChannelAttr->working_attr.repeat; pChnlPollMgr->pt_pollmgr.repeat++)
                        {
                            /* 则暂时放弃占用CPU，让优先级高的控制协程得到运�?*/
                            PT_YIELD(pt);
                            
                            pChnlPollMgr->pt_pollmgr.iErr = ChnnlPoll_CommQueryData(pChnlPollMgr, pChnlPollMgr->pt_pollmgr.pDevInfoAttr);

                            // 0 - 正常      -1 - 回应有错�?  -2 - 无回�?  -3 - 连接异常
                            if(pChnlPollMgr->pt_pollmgr.iErr == 0)
                            {						
                                /* recieve good,clear the error counter */
                                pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;
                                /* 帧间�?*/
                                timer_set(&pChnlPollMgr->pt_pollmgr.InvTimer, pChnlPollMgr->pChannelAttr->working_attr.inv_time);
                                PT_WAIT_UNTIL(pt, timer_expired(&pChnlPollMgr->pt_pollmgr.InvTimer));
                                break;
                            }

                            if(pChnlPollMgr->pt_pollmgr.iErr == -3)
                            {
                                /* 连接断开，直接设置错误次数到最大，用于下面>100 重连 */
                                pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 100;
                                break;
                            }
                            
                            /* 帧间�?*/
                            timer_set(&pChnlPollMgr->pt_pollmgr.InvTimer, pChnlPollMgr->pChannelAttr->working_attr.inv_time);
                            PT_WAIT_UNTIL(pt, timer_expired(&pChnlPollMgr->pt_pollmgr.InvTimer));
                            
                        }

                        /* recieve error pkg */
                        if(-1 == pChnlPollMgr->pt_pollmgr.iErr)
                            pChnlPollMgr->pt_pollmgr.icomErr++;
                        /* no recieve pkg */
                        else if(-2 == pChnlPollMgr->pt_pollmgr.iErr)
                            pChnlPollMgr->pt_pollmgr.icomNoRec++;
                        else if(-3 == pChnlPollMgr->pt_pollmgr.iErr)
                            pChnlPollMgr->pt_pollmgr.icomNoRec++;
                            

                        if((pChnlPollMgr->pt_pollmgr.icomNoRec == pChnlPollMgr->pt_pollmgr.icomTot) &&
                            (0 != pChnlPollMgr->pt_pollmgr.icomTot)) 
                        {
                            /* No respond */
                            fVal = 0;
                        }
                        else if((0 == pChnlPollMgr->pt_pollmgr.icomErr) && (0 == pChnlPollMgr->pt_pollmgr.icomNoRec) && (0 != pChnlPollMgr->pt_pollmgr.icomTot))
                        {
                            fVal = 1;
                            pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;
                        }
                        else
                        {
                            fVal = -1;
                            pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;
                        }
                        /* 
                            Communication Status
                        */
                        DataMgmtFops.writepoint(pChnlPollMgr->ChannelNo, pChnlPollMgr->pt_pollmgr.pDevInfoAttr->logic_id, 0, &fVal);
                    }
                }
			}

			/*
				prevent make CPU busy when POLL frame is empty
			*/
			if(pChnlPollMgr->pt_pollmgr.is_empty == true)
			{	
				usleep(10000);
				PT_YIELD(pt);
			}	
			
		}
		pChnlPollMgr->pt_pollmgr.icomChnlErrCnt++;
		if(pChnlPollMgr->pt_pollmgr.icomChnlErrCnt > 100)
		{
			pChnlPollMgr->pt_pollmgr.icomChnlErrCnt = 0;
			pChnlPollMgr->pBaseComm->disconnect(&pChnlPollMgr->com_ctx);
			sleep(3);
			pChnlPollMgr->pBaseComm->connect(&pChnlPollMgr->com_ctx);
		}
	}
	
	PT_END(pt);

}




/************************************************************************
ChnnlPoll_CommQueryCtrl(struct channelpoll_mgr_t *pChnlPollMgr)
输入参数�?
输出参数�?
返回�? �?
说明    �?
************************************************************************/
static int32_t ChnnlPoll_CommQueryCtrl(struct channelpoll_mgr_t *pChnlPollMgr, struct config_devices_info_attr *pDevInfoAttr, struct db_spl_channel_ctr_t *pctrl)
{
    typedef int32_t (*lib_control_t)(struct config_devices_info_attr *pDevInfoAttr, void *pargs, void *pbuffer, uint32_t len);

	int32_t							    nread;
    lib_control_t                       lib_control;

    if(pChnlPollMgr->pBaseComm == NULL)
	{
        return -1;
	}

    /* 清空接收缓冲�?*/
    pChnlPollMgr->pBaseComm->flush(&pChnlPollMgr->com_ctx);

	pChnlPollMgr->com_ctx.channel_no = pChnlPollMgr->ChannelNo;
	pChnlPollMgr->com_ctx.dev_no     = pDevInfoAttr->logic_id;

    if(pDevInfoAttr->phandle == NULL)
    {
        printf("lib_control failed: dll=%s dlerror=phandle is NULL\n",
               pDevInfoAttr->dll[0] ? pDevInfoAttr->dll : "(null)");
        return -1;
    }

    dlerror();
    lib_control = (lib_control_t)dlsym(pDevInfoAttr->phandle, "lib_control");
    if(lib_control == NULL)
    {
        const char *err = dlerror();
        printf("lib_control failed: dll=%s dlerror=%s\n",
               pDevInfoAttr->dll[0] ? pDevInfoAttr->dll : "(null)",
               err ? err : "(null)");
        return -1;
    }

    pChnlPollMgr->pt_ctrlmgr.send_len = lib_control(pDevInfoAttr, pctrl, pChnlPollMgr->pt_ctrlmgr.send_buff, sizeof(pChnlPollMgr->pt_ctrlmgr.send_buff));
	if(pChnlPollMgr->pt_ctrlmgr.send_len <= 0)
		return 0;

    pChnlPollMgr->pBaseComm->write(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_ctrlmgr.send_buff, pChnlPollMgr->pt_ctrlmgr.send_len);
    nread = pChnlPollMgr->pBaseComm->read(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_ctrlmgr.recv_buff, FRAME_RECVBUF_SIZE, pChnlPollMgr->pChannelAttr->working_attr.RecvTimeOut);
	if(nread > 0)
        return 0;

    else if(nread == 0)
    {
		return -2;
	}

    return -3;
}



/************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int32_t ChnnlPoll_CommQueryTrans(struct channelpoll_mgr_t *pChnlPollMgr, struct db_spl_channel_trans_t *pTrans, int timeout)
{
	int32_t							    nread;

    if(pChnlPollMgr->pBaseComm == NULL)
	{
        return -1;
	}

    /* 清空接收缓冲�?*/
    pChnlPollMgr->pBaseComm->flush(&pChnlPollMgr->com_ctx);

	pChnlPollMgr->com_ctx.channel_no = pChnlPollMgr->ChannelNo;

	/* 装载发送帧 */
	memcpy(pChnlPollMgr->pt_transmgr.send_buff, pTrans->sendbuf, pTrans->sendlen);
	pChnlPollMgr->pt_transmgr.send_len = pTrans->sendlen;

	if(pChnlPollMgr->pt_transmgr.send_len <= 0)
		return 0;

    pChnlPollMgr->pBaseComm->write(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_transmgr.send_buff, pChnlPollMgr->pt_transmgr.send_len);
	if(timeout == 0)
	{

		return 0;
	}
	else 
	{
		nread = pChnlPollMgr->pBaseComm->read(&pChnlPollMgr->com_ctx, pChnlPollMgr->pt_transmgr.recv_buff, FRAME_RECVBUF_SIZE, timeout);
		if(nread > 0)
		{
			pTrans->recvlen = nread;
			memcpy(pTrans->recvbuf, pChnlPollMgr->pt_transmgr.recv_buff, pTrans->recvlen);
			return 0;
		}
		else if(nread == 0)
		{
			return -2;
		}

		return -3;
	}
		
}


/************************************************************************
函数功能：协程�?下发控制的协�? 注意函数内部不允许使用switch
           PTCoroutine_Ctrl   下发控制�?
输入参数�?
输出参数�?
返回�? �?
说明    �?
************************************************************************/
static int32_t ChnnlPoll_PTCoroutine_Ctrl(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr)
{
    uint8_t   channelno;
	struct db_spl_channel_ctr_t       *pctrl;
    struct db_spl_realtime_data_tbl_t *pDB = pChnlPollMgr->pVmem;

	channelno = pChnlPollMgr->ChannelNo & 0x3f;

    PT_BEGIN(pt);

    while(1)
    {
		/* 本通道：循环下发直到控制队列全部发完再 YIELD，避免 485 读占满时写命令积压 */
		for (;;)
		{
			pctrl = db_ctrl_active_first(pDB, channelno);
			if (pctrl == NULL)
				break;
			if (pctrl->is_active == 1)
			{
				slist_for_each_entry(pChnlPollMgr->pt_ctrlmgr.pDevInfoAttr, &pChnlPollMgr->pChannelAttr->devices_list_head.slist, slist)
				{
					if (!pChnlPollMgr->pt_ctrlmgr.pDevInfoAttr)
						break;
					if (pChnlPollMgr->pt_ctrlmgr.pDevInfoAttr->logic_id == DB_CONVENT_ID_2_DEVNO(pctrl->db_linkid))
					{
						ChnnlPoll_CommQueryCtrl(pChnlPollMgr, pChnlPollMgr->pt_ctrlmgr.pDevInfoAttr, pctrl);
						break;
					}
				}
				pctrl->is_active = 0;
			}
		}
        PT_YIELD(pt);
	}
	PT_END(pt);

}

/************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int32_t ChnnlPoll_PTCoroutine_Trans(struct pt *pt, struct channelpoll_mgr_t *pChnlPollMgr)
{
    uint8_t   channelno;
	struct db_spl_channel_trans_t       *ptrans;
    struct db_spl_realtime_data_tbl_t *pDB = pChnlPollMgr->pVmem;
	struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

	channelno = pChnlPollMgr->ChannelNo & 0x3f;

	/* 获取有效控制信息块 */
    if(pDB->device_num <= 64)
        ptrans = &pDB->trans[channelno];
    else 
        ptrans = &pDB256->trans[channelno];

    /* A protothread function must begin with PT_BEGIN() which takes a
	     pointer to a struct pt. */
	PT_BEGIN(pt);

    while(1)
    {
		if(ptrans->is_active == 1)
		{
			ChnnlPoll_CommQueryTrans(pChnlPollMgr, ptrans, pChnlPollMgr->pChannelAttr->working_attr.RecvTimeOut);

			ptrans->is_active = 0;
		}
        PT_YIELD(pt);
	}
	PT_END(pt);
}


/*********************************************************************************************************
** Function name:       
** Descriptions:       加载动态库，创建设备
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void *ChnnlPoll_dll_load(struct config_devices_info_attr  *pDevInfoAttr)
{
	void *handle;

	handle = dlopen(pDevInfoAttr->dll, RTLD_LAZY);
	if (!handle)
	{
		fprintf(stderr, "[%s](%d) dlopen get error: %s\n", __FILE__, __LINE__, dlerror());
		return NULL;
	}

	return handle;
}



/*********************************************************************************************************
** Function name:       
** Descriptions:        遍历设备链表，创建设备
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static int ChnnlPoll_channel_dev_create(struct channelpoll_mgr_t *pChnlPollMgr)
{
	typedef int32_t(*lib_create_t)(struct config_devices_info_attr *);
	struct config_devices_info_attr  *pDevInfoAttr;

	/* 遍历本通道下所有设备，调用协议驱动so动态库 */
	slist_for_each_entry(pDevInfoAttr, &pChnlPollMgr->pChannelAttr->devices_list_head.slist, slist)
	{
		pDevInfoAttr->phandle = ChnnlPoll_dll_load(pDevInfoAttr);
		if(!pDevInfoAttr->phandle)
			continue;

		lib_create_t lib_create = (lib_create_t)dlsym(pDevInfoAttr->phandle, "lib_create");
		if(!lib_create)
			continue;
		
		pDevInfoAttr->pVmem = pChnlPollMgr->pVmem;

		lib_create(pDevInfoAttr);
	}

	return 0;
}




/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
void ChnnlPoll_channel_start(struct db_database_type *pRTDb, struct channelpoll_mgr_t *pChnlPollMgr)
{

	pChnlPollMgr->pThreadName = "ChnlPoll";
	pChnlPollMgr->ChannelNo   = pChnlPollMgr->pChannelAttr->working_attr.id;
	pChnlPollMgr->pDevName    = pChnlPollMgr->pChannelAttr->working_attr.dev;

	pChnlPollMgr->com_ctx.fd = -1;
	pChnlPollMgr->com_ctx.dev     = pChnlPollMgr->pChannelAttr->working_attr.dev;
	pChnlPollMgr->com_ctx.baud    = pChnlPollMgr->pChannelAttr->working_attr.serial_attr.baud;
	pChnlPollMgr->com_ctx.databit = pChnlPollMgr->pChannelAttr->working_attr.serial_attr.datasize;
	pChnlPollMgr->com_ctx.paritybit = pChnlPollMgr->pChannelAttr->working_attr.serial_attr.parity;
	pChnlPollMgr->com_ctx.stopbit 	= pChnlPollMgr->pChannelAttr->working_attr.serial_attr.stopbit;

	
	pChnlPollMgr->com_ctx.keepidle = 60;
	pChnlPollMgr->com_ctx.keepintvl = 60;
	pChnlPollMgr->com_ctx.keepcnt = 3;
	pChnlPollMgr->com_ctx.pvmem = pRTDb;
	pChnlPollMgr->com_ctx.dump = pChnlPollMgr->poptarg->dump;
	/*
			通道下设备点表创建
		*/
	ChnnlPoll_channel_dev_create(pChnlPollMgr);


	if(strcmp(pChnlPollMgr->pChannelAttr->working_attr.link, "serial") == 0)
	{
		pChnlPollMgr->pBaseComm = (struct sampling_com_baseclass_ops_t *)sampling_com_serial_ops_get();
	}
	else if(strcmp(pChnlPollMgr->pChannelAttr->working_attr.link, "tcp client") == 0)
	{
		pChnlPollMgr->pBaseComm = (struct sampling_com_baseclass_ops_t *)sampling_com_tcpclient_ops_get();
	}

	/*
		only start poll when checked == 1
	*/

}


