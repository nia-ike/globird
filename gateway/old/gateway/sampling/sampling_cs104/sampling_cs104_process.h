// sampling_channelpoll.h: interface for the CChannelPoll class.
//
//////////////////////////////////////////////////////////////////////

#ifndef _SAMPLING_CS104_PROCESS_H_
#define _SAMPLING_CS104_PROCESS_H_


#include "pt.h"
#include "sampling_com_baseclass.h"
#include "sampling_cfg.h"
#include "db_struct.h"
#include "cs104_connection.h"



/*
	channel poll manager struct .
	it contain the channel working parameter.
*/
struct cs104_mgr_t
{
	/* channel attribute */
	struct config_channel_attr  ChannelAttr;
	
	/* iec104*/
	CS104_Connection 			 con;

	/* 
		this channel number.
	*/
	int32_t 					ChannelNo;
		

	

	/*
		realtime database address
	*/
	struct db_spl_realtime_data_tbl_t *pVmem;

	/*

	*/
	struct sampling_optarg_t    	  *poptarg;

};



int cs104_channel_process(struct cs104_mgr_t *pChnlCs104Mgr);
void cs104_channel_start(struct db_spl_realtime_data_tbl_t *pRTDb, struct cs104_mgr_t  *pChnlCs104Mgr);

#endif // #ifndef _CHANNEL_POLL_H_
