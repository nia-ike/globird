/*
*  Copyright (c) 2019,广州碳索智控科技有限公司
*  All rights reserved.
* 
*  文件名称：
*  文件标识：
*  描    述：
* 
*  当前版本：V1.0
*  作    者：chenzy
*  运行环境：
*  操作系统: linux
*  硬件平台：
*  固件库  ：
*  修改记录 
*  作者       时间         版本      修改描述
*  chenzy          V1.00      
*/


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>  //stat
#include <time.h>
#include <stdbool.h>
#include <pthread.h>  // Add this header for thread support

// ... existing code ...
#include <signal.h>

// Add open62541 header
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include <open62541/plugin/log_stdout.h>

#include "db_node_fops.h"
#include "db_device.h"
#include "db_struct.h"

#include "opc_ua_cfg.h"

// Global variable for server
static pthread_t opc_ua_update_threadid;  // Add thread handle




/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static int opc_ua_addDataPoints(struct config_chnl_working_attr *pCfgChlAttr, UA_Server *server)
{
    char   foldername[64];
    struct db_node_list_t *pNode_attr;
    struct db_device_list_t *pDev_attr;
    struct db_device_list_t *pdev_list_head;

    UA_Double lfval;
    UA_NodeId folderId;
    UA_ObjectAttributes folderAttr = UA_ObjectAttributes_default;
    UA_VariableAttributes varAttr = UA_VariableAttributes_default;

    // 遍历下挂设备，以及该设备下的测点
    pdev_list_head = &pCfgChlAttr->device_list;
    slist_for_each_entry(pDev_attr, &pdev_list_head->list, list)
    {
        snprintf(foldername, sizeof(foldername), "Device:C%03d_D%03d", pDev_attr->id.split.ucnPort, pDev_attr->id.split.ucnDev);
        folderAttr.displayName = UA_LOCALIZEDTEXT("en-US", foldername);
        UA_Server_addObjectNode(server, UA_NODEID_NULL,
                          UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES),
                          UA_QUALIFIEDNAME(1, foldername),
                          UA_NODEID_NUMERIC(0, UA_NS0ID_FOLDERTYPE),
                          folderAttr, NULL, &folderId);

        slist_for_each_entry(pNode_attr, &pDev_attr->node_list.list, list)
        {
            // 读取数值
            DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);

            UA_Variant_setScalar(&varAttr.value, &lfval, &UA_TYPES[UA_TYPES_DOUBLE]);
            varAttr.displayName = UA_LOCALIZEDTEXT("en-US", pNode_attr->name);
            varAttr.accessLevel = UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE;
            UA_Server_addVariableNode(server, UA_NODEID_STRING(1, pNode_attr->name),
                                    folderId,
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_HASCOMPONENT),
                                    UA_QUALIFIEDNAME(1, pNode_attr->name),
                                    UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                                    varAttr, NULL, &pNode_attr->UA_nodeId);

        }
    }

    return 0;

}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:           Update thread function to update OPC UA values                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
static void* opc_ua_data_update_process(void* arg) 
{
    struct db_node_list_t *pNode_attr;
    struct db_device_list_t *pDev_attr;
    
    struct config_chnl_working_attr *pCfgChlAttr = (struct config_chnl_working_attr *)arg;
    struct db_device_list_t *pdev_list_head = &pCfgChlAttr->device_list; 

    UA_Variant ua_value;
    double     lfval;

    while(1)
    {
        slist_for_each_entry(pDev_attr, &pdev_list_head->list, list)
        {
            slist_for_each_entry(pNode_attr, &pDev_attr->node_list.list, list)
            {
                // 读取数值
                DataMgmtFops.readpoint_by_id(pNode_attr->db_link, &lfval);

                UA_Variant_init(&ua_value);
                UA_Variant_setScalar(&ua_value, &lfval, &UA_TYPES[UA_TYPES_DOUBLE]);
                UA_Server_writeValue(pCfgChlAttr->ua_server, pNode_attr->UA_nodeId, ua_value);
            }
        }

        sleep(1);
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
int opc_ua_server_init(struct config_chnl_working_attr *pCfgChlAttr)
{
    // Initialize OPC UA server
    pCfgChlAttr->ua_server = UA_Server_new();
    if (!pCfgChlAttr->ua_server) {
        printf("Failed to create OPC UA server\n");
        return -1;
    }

    // Configure server
    UA_ServerConfig *config = UA_Server_getConfig(pCfgChlAttr->ua_server);
    
    // set listen port
    UA_ServerConfig_setMinimal(config, pCfgChlAttr->opc_ua_comm_ctx.port, NULL);

    // Set application name
    UA_String_clear(&config->applicationDescription.applicationName.text);
    config->applicationDescription.applicationName = UA_LOCALIZEDTEXT("en-US", "OPC UA Server");

    // Set port from command line if specified
 //   config->networkLayers[0].discoveryUrl.data = (UA_Byte*)UA_malloc(64);
 //   snprintf((char*)config->networkLayers[0].discoveryUrl.data, 64, "opc.tcp://0.0.0.0:%d", pCfgChlAttr->opc_ua_comm_ctx.port);
 //   config->networkLayers[0].discoveryUrl.length = strlen((char*)config->networkLayers[0].discoveryUrl.data);

    // Add data points after server initialization but before running
    opc_ua_addDataPoints(pCfgChlAttr, pCfgChlAttr->ua_server);

    // Create update thread before starting server
    if (pthread_create(&opc_ua_update_threadid, NULL, opc_ua_data_update_process, pCfgChlAttr) != 0) {
        printf("Failed to create update thread\n");
        UA_Server_delete(pCfgChlAttr->ua_server);
        return -1;
    }

    // Start OPC UA server
    UA_StatusCode retval = UA_Server_run_startup(pCfgChlAttr->ua_server);
    if(retval != UA_STATUSCODE_GOOD) {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Server startup failed");
        UA_Server_delete(pCfgChlAttr->ua_server);
        
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
int opc_ua_server_destory(struct config_chnl_working_attr *pCfgChlAttr)
{
    // Wait for update thread to finish
    pthread_join(opc_ua_update_threadid, NULL);
    // Cleanup
    UA_Server_run_shutdown(pCfgChlAttr->ua_server);
    UA_Server_delete(pCfgChlAttr->ua_server);

    return 0;
}

/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:                                         **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
void opc_ua_process(struct config_chnl_working_attr *pCfgChlAttr)
{
    while(1)
    {
        UA_Server_run_iterate(pCfgChlAttr->ua_server, true);
    }
        
}