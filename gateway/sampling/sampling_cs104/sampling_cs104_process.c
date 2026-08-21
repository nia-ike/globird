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
#include "sampling_com_baseclass.h"
#include "db_node_fops.h"
#include "debug.h"
#include "db_struct.h"
#include "db_device.h"
#include "dev_log.h"

#include <unistd.h>
#include "./lib/crc16.h"

#include "sampling_cs104_process.h"
#include "db_node_fops.h"
#include "config_devices.h"
#include "cs101_information_objects.h"
#include "iec60870_common.h"
#include "iec104_cfg.h"
#include "cs104_connection.h"

#define Interrogation_inv 60  // 10 s


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/


static bool running = true;

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
void sigint_handler(int signalId)
{
    running = false;
}

static CS104_ConnectionEvent lastEvent = -1;

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void rawMessageHandler (void* parameter, uint8_t* msg, int msgSize, bool sent)
{
	struct cs104_mgr_t *pChnlCs104Mgr = (struct cs104_mgr_t *)parameter;

	if(pChnlCs104Mgr->poptarg->dump)
	{
		if (sent)
			printf("SEND cnnl%d: ", pChnlCs104Mgr->ChannelNo);
		else
			printf("RCVD chnl%d: ", pChnlCs104Mgr->ChannelNo);

		int i;
		for (i = 0; i < msgSize; i++) {
			printf("%02x ", msg[i]);
		}

    	printf("\n");
	}

    if (sent) {
        log_write(pChnlCs104Mgr->ChannelNo, 0, 1, msg, msgSize);
    } else {
        log_write(pChnlCs104Mgr->ChannelNo, 0, 0, msg, msgSize);
    }
}


/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static void connectionHandler (void* parameter, CS104_Connection connection, CS104_ConnectionEvent event)
{
	double fval;
	struct cs104_mgr_t *pChnlCs104Mgr = (struct cs104_mgr_t *)parameter;
	struct config_channel_attr *pChannelAttr = (struct config_channel_attr *)&pChnlCs104Mgr->ChannelAttr;
	struct config_devices_info_attr *pDevInfoAttr;

    switch (event) {
    case CS104_CONNECTION_OPENED:
        printf("Connection established\n");
		fval = 1;
		slist_for_each_entry(pDevInfoAttr, &pChannelAttr->devices_list_head.slist, slist)
		{
			DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, 0, &fval);
		}
        break;
    case CS104_CONNECTION_CLOSED:
        printf("Connection closed\n");
		fval = 0;
		slist_for_each_entry(pDevInfoAttr, &pChannelAttr->devices_list_head.slist, slist)
		{
			DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, 0, &fval);
		}
        break;
    case CS104_CONNECTION_STARTDT_CON_RECEIVED:
        printf("Received STARTDT_CON\n");
        break;
    case CS104_CONNECTION_STOPDT_CON_RECEIVED:
        printf("Received STOPDT_CON\n");
        break;
    case CS104_CONNECTION_FAILED:
        printf("Connection failed\n");
		fval = 0;
		slist_for_each_entry(pDevInfoAttr, &pChannelAttr->devices_list_head.slist, slist)
		{
			DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, 0, &fval);
		}
        break;
    }

    lastEvent = event;
}

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
static bool asduReceivedHandler (void* parameter, int address, CS101_ASDU asdu)
{
	int 	i;
	int 	ioa;
	double 	value;
	int 	index;
	struct cs104_mgr_t *pChnlCs104Mgr = (struct cs104_mgr_t *)parameter;
	struct config_devices_info_attr *pDevInfoAttr;
	struct config_node_attr *pnode_attr;

	if(pChnlCs104Mgr->poptarg->dump)
	{
		printf("RECVD ASDU type: %s(%i) elements: %i\n",
            TypeID_toString(CS101_ASDU_getTypeID(asdu)),
            CS101_ASDU_getTypeID(asdu),
            CS101_ASDU_getNumberOfElements(asdu));

		// 特别提示计数器相关的报文
		if (CS101_ASDU_getTypeID(asdu) == M_IT_NA_1 || 
			CS101_ASDU_getTypeID(asdu) == M_IT_TA_1 || 
			CS101_ASDU_getTypeID(asdu) == M_IT_TB_1) {
			printf(">>> This is a COUNTER data response! <<<\n");
		}
	}

    if (CS101_ASDU_getTypeID(asdu) == M_ME_TE_1) {

        int i;

        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) {

            MeasuredValueScaledWithCP56Time2a io =
                    (MeasuredValueScaledWithCP56Time2a) CS101_ASDU_getElement(asdu, i);

//           printf("    IOA: %i value: %i\n",
//                   InformationObject_getObjectAddress((InformationObject) io),
//                   MeasuredValueScaled_getValue((MeasuredValueScaled) io)
//           );

            MeasuredValueScaledWithCP56Time2a_destroy(io);
        }
    }
    
    else if (CS101_ASDU_getTypeID(asdu) == C_TS_TA_1) {
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_ME_NA_1) {
        

        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            MeasuredValueNormalized io =
                    (MeasuredValueNormalized) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			value = MeasuredValueNormalized_getValue((MeasuredValueNormalized) io);

			//--------------
//			printf("    IOA: %i value: %f\n",
//                    InformationObject_getObjectAddress((InformationObject) io),
//                    MeasuredValueNormalized_getValue((MeasuredValueNormalized) io)
//            );

			//---------------------

			MeasuredValueNormalized_destroy(io);

			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				value = value * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &value);
				break;
			}
		 	
        }
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_ME_NB_1) {
        
        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            MeasuredValueScaled io =
                    (MeasuredValueScaled) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			value = MeasuredValueScaled_getValue((MeasuredValueScaled) io);

			MeasuredValueScaled_destroy(io);

			//  南向时，0 已经用于虚拟的通讯状态点。标度化的0 暂时忽略，留给通讯状态。实际上是暂时不支持标度化数据
			if(ioa == 0)
				continue;

			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				value = value * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &value);
				break;
			}
			
        }
    }
    else if (CS101_ASDU_getTypeID(asdu) == M_ME_NC_1) {
        
        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            MeasuredValueShort io =
                    (MeasuredValueShort) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			value = MeasuredValueShort_getValue((MeasuredValueShort) io);

			MeasuredValueShort_destroy(io);

			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				value = value * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &value);
				break;
			}
        }
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_IT_NA_1) {
        // 处理计数器数据响应（累计量）
        
        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            IntegratedTotals io = (IntegratedTotals) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			BinaryCounterReading bcr = IntegratedTotals_getBCR(io);
			int32_t counterValue = BinaryCounterReading_getValue(bcr);

			// 在这里处理计数器数据，写入数据库等
			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				double dvalue = (double)counterValue * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &dvalue);
				break;
			}

			IntegratedTotals_destroy(io);
        }
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_IT_TA_1) {
        // 处理带时间戳的计数器数据响应        
        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            IntegratedTotalsWithCP24Time2a io = (IntegratedTotalsWithCP24Time2a) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			BinaryCounterReading bcr = IntegratedTotals_getBCR((IntegratedTotals)io);
			int32_t counterValue = BinaryCounterReading_getValue(bcr);

			// 处理计数器数据
			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				double dvalue = (double)counterValue * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &dvalue);
				break;
			}

			IntegratedTotalsWithCP24Time2a_destroy(io);
        }
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_IT_TB_1) {
        // 处理带CP56Time2a时间戳的计数器数据响应
        
        for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            IntegratedTotalsWithCP56Time2a io = (IntegratedTotalsWithCP56Time2a) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			BinaryCounterReading bcr = IntegratedTotals_getBCR((IntegratedTotals)io);
			int32_t counterValue = BinaryCounterReading_getValue(bcr);


			// 处理计数器数据
			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				double dvalue = (double)counterValue * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &dvalue);
				break;
			}

			IntegratedTotalsWithCP56Time2a_destroy(io);
        }
    }
	else if (CS101_ASDU_getTypeID(asdu) == M_SP_NA_1) {
       
		for (i = 0; i < CS101_ASDU_getNumberOfElements(asdu); i++) 
		{
            SinglePointInformation io =
                    (SinglePointInformation) CS101_ASDU_getElement(asdu, i);

			ioa = InformationObject_getObjectAddress((InformationObject) io);
			value = SinglePointInformation_getValue((SinglePointInformation) io);

			SinglePointInformation_destroy(io);

			slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
			{
				index = mb_get_nodeattr_by_infaddr(&pDevInfoAttr->DevAttr.node_attr[0], 
						ioa, 
						0,
						pDevInfoAttr->DevAttr.node_num);

				if(index < 0)
					continue;

				pnode_attr = &pDevInfoAttr->DevAttr.node_attr[index];
				value = value * pnode_attr->kfactor;
				DataMgmtFops.writepoint(pDevInfoAttr->channel_id, pDevInfoAttr->logic_id, pnode_attr->id, &value);
				break;
			}
         
        }
    }

    return true;
}

// 函数声明
static bool cs104_send_do_command(struct cs104_mgr_t *pChnlCs104Mgr, struct config_control_attr *pCtrlAttr, double value);
static bool cs104_send_ao_command(struct cs104_mgr_t *pChnlCs104Mgr, struct config_control_attr *pCtrlAttr, double value);
static int cs104_process_control(struct cs104_mgr_t *pChnlCs104Mgr, struct db_spl_channel_ctr_t *pctrl);

/*********************************************************************************************************
** Function name:       
** Descriptions:        
** input parameters:             
** output parameters:   NONE
** Returned value:      
*********************************************************************************************************/
int cs104_channel_process(struct cs104_mgr_t *pChnlCs104Mgr)
{
	bool ret;
	bool startDTSent;

    uint32_t lastGiSent = 0;
    uint32_t lastCounterSent = 0;
    uint32_t giStartForCounter = 0;

	while(1)
	{
		// 循环 进行连接
		ret = CS104_Connection_connect(pChnlCs104Mgr->con);
		if(ret == false)
		{
			sleep(1);
			continue;
		}
		
		startDTSent = false;
		lastGiSent = 0;
		lastCounterSent = 0;
		//连接成功后进行数据处理
		while (1)
		{
			if (lastEvent == CS104_CONNECTION_OPENED) {
				CS104_Connection_sendStartDT(pChnlCs104Mgr->con);
				startDTSent = true;
			}
			else if (lastEvent == CS104_CONNECTION_CLOSED || lastEvent == CS104_CONNECTION_FAILED) {
				break; // 断开连接，返回上一个循环，尝试连接
			}
			else
			{
				if (startDTSent)
				{
					uint32_t currentTime = time(NULL);

					// 优先处理控制队列，确保控制及时响应
					// 如果队列中有多条控制命令，连续处理，缩短间隔
					{
						uint8_t channelno;
						struct db_spl_channel_ctr_t *pctrl;
						struct db_spl_realtime_data_tbl_t *pDB = pChnlCs104Mgr->pVmem;
						
						channelno = pChnlCs104Mgr->ChannelNo & 0x3f;
						
						// 连续处理所有待处理的控制命令
						// 队列中有多条时，使用较短间隔（10ms）连续处理
						// 队列处理完后，退出循环，恢复正常100ms间隔
						while(1)
						{
							pctrl = db_ctrl_active_first(pDB, channelno);
							
							if(pctrl != NULL && pctrl->is_active == 1)
							{
								cs104_process_control(pChnlCs104Mgr, pctrl);
								pctrl->is_active = 0;  // 标记为已处理
								
								// 控制命令之间使用较短延时（10ms），快速处理队列
								usleep(10000);
							}
							else
							{
								// 队列处理完，退出循环
								break;
							}
						}
					}

					// 处理总召唤和计数器查询（使用时间戳控制，不阻塞）
					if (currentTime < lastGiSent)
						lastGiSent = currentTime;

					// 发送总召唤命令
					if (currentTime >= lastGiSent + Interrogation_inv) {
						lastGiSent = currentTime;
						CS104_Connection_sendInterrogationCommand(pChnlCs104Mgr->con, CS101_COT_ACTIVATION, 1, IEC60870_QOI_STATION);

                        /*
                         * 计数器总召（累计量）应在一次总召之后延时触发。
                         * 注意：总召可能会周期性发送（例如10秒一次），不能用 lastGiSent 作为
                         * “计数器触发基准”，否则 lastGiSent 会不断刷新，永远达不到 +30s。
                         */
                        if (giStartForCounter == 0)
                            giStartForCounter = currentTime;
					}

					// 在总召唤后30秒发送计数器查询（使用时间戳，不阻塞）
					if (currentTime < lastCounterSent)
						lastCounterSent = currentTime;

					if (giStartForCounter > 0 && currentTime >= giStartForCounter + 30 &&
					    (lastCounterSent == 0 || currentTime >= lastCounterSent + Interrogation_inv)) {
						lastCounterSent = currentTime;
						// 使用正确的QCC参数：读取全部计数器组
						uint8_t qcc = IEC60870_QCC_RQT_GENERAL + IEC60870_QCC_FRZ_READ;  // 5 + 0x00 = 5
						ret = CS104_Connection_sendCounterInterrogationCommand(pChnlCs104Mgr->con, CS101_COT_ACTIVATION, 1, qcc);
						if(ret == false)
						{
							printf("sendCounterInterrogationCommand failed\n");
						}
						else 
						{
							printf("sendCounterInterrogationCommand success, waiting for M_IT_NA_1 response...\n");
						}

                        /* 本轮计数器触发完成，等待下一次GI重新触发 */
                        giStartForCounter = 0;
					}
				}
			}

			usleep(100000);
		}

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
void cs104_channel_start(struct db_spl_realtime_data_tbl_t *pRTDb, struct cs104_mgr_t *pChnlCs104Mgr)
{
	struct config_channel_attr  *pChannelAttr = &pChnlCs104Mgr->ChannelAttr;
    struct config_devices_info_attr *pDevInfoAttr = NULL;
    int cotSize = 2;

	pChnlCs104Mgr->con = CS104_Connection_create(pChannelAttr->working_attr.dst_url, pChannelAttr->working_attr.dst_port);//
    CS104_APCIParameters apciParameters = CS104_Connection_getAPCIParameters(pChnlCs104Mgr->con);
    apciParameters->t0 = 2;

    CS101_AppLayerParameters alParams = CS104_Connection_getAppLayerParameters(pChnlCs104Mgr->con);
    alParams->sizeOfCOT = cotSize;
    alParams->originatorAddress = 3;

	CS104_Connection_setConnectionHandler(pChnlCs104Mgr->con, connectionHandler, pChnlCs104Mgr);
	CS104_Connection_setASDUReceivedHandler(pChnlCs104Mgr->con, asduReceivedHandler, pChnlCs104Mgr);

	
	/* uncomment to log messages */
	CS104_Connection_setRawMessageHandler(pChnlCs104Mgr->con, rawMessageHandler, pChnlCs104Mgr);

}

/*********************************************************************************************************
** Function name:       cs104_send_do_command
** Descriptions:        发送 DO（单点/双点遥控）命令
** input parameters:    pChnlCs104Mgr 通道管理器
**                      pCtrlAttr 控制属性
**                      value 控制值（0=断开，1=闭合）
** output parameters:   NONE
** Returned value:      成功返回true，失败返回false
*********************************************************************************************************/
static bool cs104_send_do_command(struct cs104_mgr_t *pChnlCs104Mgr, struct config_control_attr *pCtrlAttr, double value)
{
    bool commandValue = (value != 0.0);  // 转换为 bool：0=断开，非0=闭合
    bool ret;
    
    if(pCtrlAttr->double_ctrl)
    {
        // 双点遥控 C_DC_NA_1 (46)
        // 双点命令：0=不允许，1=OFF，2=ON，3=不允许
        int dcq_value = commandValue ? 2 : 1;  // 2=ON, 1=OFF
        DoubleCommand dc = DoubleCommand_create(NULL, pCtrlAttr->inf_addr, dcq_value, false, 0);
        
        ret = CS104_Connection_sendProcessCommandEx(pChnlCs104Mgr->con, CS101_COT_ACTIVATION, 1, (InformationObject)dc);
        
        DoubleCommand_destroy(dc);
    }
    else
    {
        // 单点遥控 C_SC_NA_1 (45)
        SingleCommand sc = SingleCommand_create(NULL, pCtrlAttr->inf_addr, commandValue, false, 0);
        
        ret = CS104_Connection_sendProcessCommandEx(pChnlCs104Mgr->con, CS101_COT_ACTIVATION, 1, (InformationObject)sc);
        
        SingleCommand_destroy(sc);
    }
    
    return ret;
}

/*********************************************************************************************************
** Function name:       cs104_send_ao_command
** Descriptions:        发送 AO（浮点数遥调）命令
** input parameters:    pChnlCs104Mgr 通道管理器
**                      pCtrlAttr 控制属性
**                      value 控制值
** output parameters:   NONE
** Returned value:      成功返回true，失败返回false
*********************************************************************************************************/
static bool cs104_send_ao_command(struct cs104_mgr_t *pChnlCs104Mgr, struct config_control_attr *pCtrlAttr, double value)
{
    // 应用 kfactor 系数
    float setpointValue = (float)(value / pCtrlAttr->kfactor);
    
    // 短浮点数设定值命令 C_SE_NC_1 (50)
    SetpointCommandShort sc = SetpointCommandShort_create(NULL, pCtrlAttr->inf_addr, setpointValue, false, 0);
    
    bool ret = CS104_Connection_sendProcessCommandEx(pChnlCs104Mgr->con, CS101_COT_ACTIVATION, 1, (InformationObject)sc);
    
    SetpointCommandShort_destroy(sc);
    
    return ret;
}

/*********************************************************************************************************
** Function name:       cs104_process_control
** Descriptions:        处理控制命令
** input parameters:    pChnlCs104Mgr 通道管理器
**                      pctrl 控制信息块
** output parameters:   NONE
** Returned value:      成功返回0，失败返回-1
*********************************************************************************************************/
static int cs104_process_control(struct cs104_mgr_t *pChnlCs104Mgr, struct db_spl_channel_ctr_t *pctrl)
{
    struct config_devices_info_attr *pDevInfoAttr;
    struct config_control_attr *pCtrlAttr;
    uint32_t ctrlType;
    uint8_t ctrlIndex;
    bool ret;
    
    // 从 db_linkid 提取控制类型和索引
    ctrlType = DB_CONVENT_ID_2_TYPE(pctrl->db_linkid);
    ctrlIndex = DB_CONVENT_ID_2_INDEX(pctrl->db_linkid);
    
    // 遍历设备列表，查找匹配的设备
    slist_for_each_entry(pDevInfoAttr, &pChnlCs104Mgr->ChannelAttr.devices_list_head.slist, slist)
    {
        if(pDevInfoAttr->logic_id == DB_CONVENT_ID_2_DEVNO(pctrl->db_linkid))
        {
            // 查找控制属性
            pCtrlAttr = iec104_cfg_control_attr(&pDevInfoAttr->DevAttr, ctrlIndex);
            if(pCtrlAttr == NULL)
            {
                printf("cs104_process_control: control attr not found for id=%d\n", ctrlIndex);
                return -1;
            }
            
            // 根据控制类型发送相应的命令
            if(ctrlType == DB_TYPE_DO)
            {
                // DO 单点/双点遥控
                ret = cs104_send_do_command(pChnlCs104Mgr, pCtrlAttr, pctrl->value);
                if(ret)
                {
                    if(pChnlCs104Mgr->poptarg->dump)
                    {
                        printf("cs104_send_do_command: success, inf_addr=%d, value=%.2f\n", 
                               pCtrlAttr->inf_addr, pctrl->value);
                    }
                }
                else
                {
                    printf("cs104_send_do_command: failed, inf_addr=%d\n", pCtrlAttr->inf_addr);
                    return -1;
                }
            }
            else if(ctrlType == DB_TYPE_AO)
            {
                // AO 浮点数遥调
                ret = cs104_send_ao_command(pChnlCs104Mgr, pCtrlAttr, pctrl->value);
                if(ret)
                {
                    if(pChnlCs104Mgr->poptarg->dump)
                    {
                        printf("cs104_send_ao_command: success, inf_addr=%d, value=%.2f, kfactor=%.2f\n", 
                               pCtrlAttr->inf_addr, pctrl->value, pCtrlAttr->kfactor);
                    }
                }
                else
                {
                    printf("cs104_send_ao_command: failed, inf_addr=%d\n", pCtrlAttr->inf_addr);
                    return -1;
                }
            }
            else
            {
                printf("cs104_process_control: unsupported control type 0x%03x\n", ctrlType);
                return -1;
            }
            
            return 0;
        }
    }
    
    printf("cs104_process_control: device not found for logic_id=%d\n", DB_CONVENT_ID_2_DEVNO(pctrl->db_linkid));
    return -1;
}


