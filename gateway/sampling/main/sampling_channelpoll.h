// sampling_channelpoll.h: interface for the CChannelPoll class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SAMPLING_CHANNEL_POLL_H_
#define _SAMPLING_CHANNEL_POLL_H_


#include "pt.h"
#include "sampling_com_baseclass.h"
#include "sampling_cfg.h"
#include "db_struct.h"

/*
	channel poll "POLL PARAM" in libary -"PT" 
*/
struct channelpoll_poll_mgr
{
	/*										 
		data frame manage information table.
	*/
	struct config_devices_info_attr *pDevInfoAttr;
	struct config_request_list 		*pCurRequestlist;

	uint8_t                 		recv_buff[FRAME_RECVBUF_SIZE];                                                     // 接收数据包缓冲区
    int16_t                 		recv_len; 														 // 接收数据包缓冲区长度

	int16_t							iErr;
	int16_t							icomErr;
	int16_t							icomTot;
	int16_t							icomNoRec;
	uint16_t						icomChnlErrCnt;  /*  */
	
	uint8_t							is_empty;

	int16_t							repeat;
	struct timer					InvTimer; 			/* 帧间隔等待定时器 */
	struct timer					InvPollTimer;		/* 轮训间隔定时器 */
};


/*
	channel poll "POLL PARAM" in libary -"PT" 
*/
struct channelpoll_ctrl_mgr
{
	/*										 
		data frame manage information table.
	*/
	struct config_devices_info_attr *pDevInfoAttr;

	uint8_t                 		recv_buff[FRAME_RECVBUF_SIZE];                                                     // 接收数据包缓冲区
    int16_t                 		recv_len; 														 // 接收数据包缓冲区长度

    uint8_t                 		send_buff[FRAME_RECVBUF_SIZE];                                                     // 接收数据包缓冲区
    int16_t                 		send_len; 					

	int16_t							repeat;
};


/*
	channel poll "POLL PARAM" in libary -"PT" 
*/
struct channelpoll_trans_mgr
{
	/*										 
		data frame manage information table.
	*/
	struct config_devices_info_attr *pDevInfoAttr;

	uint8_t                 		recv_buff[FRAME_RECVBUF_SIZE];                                                     // 接收数据包缓冲区
    int16_t                 		recv_len; 														 // 接收数据包缓冲区长度

    uint8_t                 		send_buff[FRAME_RECVBUF_SIZE];                                                     // 接收数据包缓冲区
    int16_t                 		send_len; 			
};

/*
	channel poll manager struct .
	it contain the channel working parameter.
*/
struct channelpoll_mgr_t
{
	/* channel attribute */
	struct config_channel_attr  *pChannelAttr;
	/*
		phy interface name . for example /dev/uart1 
	*/
	char						*pDevName;

	/*
		this channel poll thread name
	*/
	char						*pThreadName;

	/* 
		this channel number.
	*/
	int32_t 					ChannelNo;
		

	/*
		communication base class. 
	*/
	struct sampling_com_baseclass_ops_t 	*pBaseComm;
	

	
	/*
		if use RS485, this struct is manage the serial parameter.
		if use socket, this will not effect.
	*/
	struct sampling_comm_ctx_t		com_ctx;
    

	/*
		please see the protothread(PT) used.
		the PT need the "static" or "globol" var for work.
	*/
	struct channelpoll_poll_mgr		pt_pollmgr;

    struct channelpoll_ctrl_mgr     pt_ctrlmgr;

	struct channelpoll_trans_mgr    pt_transmgr;

	/*
		realtime database address
	*/
	struct db_spl_realtime_data_tbl_t *pVmem;

	/*

	*/
	struct sampling_optarg_t    	  *poptarg;
};



void *ChnnlPoll_thread_entry(void* parameter);
void ChnnlPoll_channel_start(struct db_database_type *pRTDb, struct channelpoll_mgr_t *pChnlPollMgr);


#endif // #ifndef _CHANNEL_POLL_H_
