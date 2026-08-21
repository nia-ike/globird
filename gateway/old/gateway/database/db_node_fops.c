/*
* Copyright (c) 2013,珠海一构物联有限公司
* All rights reserved.
*
* 文件名称：db_cfg.h
* 文件标识：
* 描    述：
*
* 当前版本：V1.0
* 作    者：chenzy
* 更改说明：
* 完成日期：20131108
*
* 修改记录
*  作者       时间         版本      修改描述
*  chenzy   2013.11.08   V1.00      创建
*/
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "db_node_fops.h"

#define DB_NODE_MAX             8192


/*************************************************************************/
/** Function Name:                                     **/
/** Purpose:            输入参数为NULL 时，只Get获取实时数据对象
					    输入参数不为NULL时， 设置 实时数据对象。用于初始化时，传递共享内存的首地址
                              **/
/** Params:            None.                                               **/
/** Return:            None.                                               **/
/** Notice:            None.                                               **/
/*************************************************************************/
struct db_spl_realtime_data_tbl_t *db_DataTblObj(struct db_spl_realtime_data_tbl_t *p)
{
	static struct db_spl_realtime_data_tbl_t *database = NULL;

	if(p!= NULL)
		database = p;

	return database;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static int32_t db_init(void *pDB)
{
    /* install the pDB */
    db_DataTblObj(pDB);

    return 0;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static uint32_t db_map_node_num(void)
{
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);

    return pDB->node_num;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static uint32_t db_map_dev_num(void)
{
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);

    return pDB->device_num;
}

/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static uint32_t db_map_active_node_size(void)
{
    uint32_t length, node_num;

    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);

    /* 限制总条数 */
    node_num = (pDB->node_num > DB_NODE_MAX) ? DB_NODE_MAX : pDB->node_num;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        /* 索引表长度 */
        length = offsetof(struct db_spl_realtime_data_tbl_t, node);
    }
    else 
    {
        /* 索引表长度 */
        length = offsetof(struct db_spl_realtime_data_tbl_256_t, node);
    }
    

    /* 总长度 = 索引长度 + n x node长度 */
    if(node_num > 0)
        length = length + node_num * sizeof(struct db_spl_realtime_data_node_t);
    
    return length;
}


/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_spl_remap_info_t *db_getIndexDevBySeq(uint16_t i) 
{
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

    if(i > DB_CFG_MAX_DEV)
        return NULL;

    if(i > pDB->device_num)
        return NULL;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        return &pDB->index[i];
    }
    else 
    {
        return &pDB256->index[i];
    }    
}



/*************************************************************************/
/** Function Name:                                                     **/    
/** Purpose:                                                           **/    
/** Params:            None.                                           **/    
/** Return:            None.                                           **/    
/** Notice:            None.                                           **/    
/*************************************************************************/
static struct db_spl_remap_info_t *db_getIndexByPortAndDev(uint8_t ucnPort, uint8_t ucnDev) 
{
	int 		i;
	uint16_t 	id;
	
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

    id = DB_CONVENT_CHNLDEV_GROUP(ucnPort, ucnDev);

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        for(i=0; i<pDB->device_num; i++)
        {
            if(pDB->index[i].id.mix == id)
            {
                return &pDB->index[i];
            }
        }
    }
    else 
    {
        for(i=0; i<pDB256->device_num; i++)
        {
            if(pDB256->index[i].id.mix == id)
            {
                return &pDB256->index[i];
            }
        }
    }   

	return NULL;
}

/* -------------------------- 控制点索引/读写接口（独立于采集点） -------------------------- */
static struct db_spl_remap_info_t *db_getCtrlIndexByPortAndDev(uint8_t ucnPort, uint8_t ucnDev)
{
    int i;
    uint16_t id;
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_ctrl_tbl_t *pCtrl;

    if(!pDB)
        return NULL;

    pCtrl = &pDB->ctrl_node_region[0].context;
    id = DB_CONVENT_CHNLDEV_GROUP(ucnPort, ucnDev);

    for(i=0; i<pCtrl->device_num; i++)
    {
        if(pCtrl->index[i].id.mix == id)
        {
            return &pCtrl->index[i];
        }
    }

    return NULL;
}

static int32_t db_readctrlpoint(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value)
{
    struct db_spl_remap_info_t *pIndex;
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_ctrl_tbl_t *pCtrl;
    struct db_spl_realtime_data_node_t *pNode;

    if((pDB == NULL) || (f64Value == NULL))
        return -1;

    pIndex = db_getCtrlIndexByPortAndDev(ucnPort, ucnDev);
    if(!pIndex)
        return -1;

    if(ucnIndex >= pIndex->node_num)
        return -1;

    pCtrl = &pDB->ctrl_node_region[0].context;
    pNode = &pCtrl->node[0];
    pNode = pNode + pIndex->index;

    *f64Value = ((double)pNode[ucnIndex].value_now) / 1000;
    return 0;
}

static int32_t db_readctrlpoint_by_id(uint32_t ul32ID, double *f64Value)
{
    uint16_t type = DB_CONVENT_ID_2_TYPE(ul32ID);


    /* 控制点接口只接受 DO/AO 类型 */
    if((type != DB_TYPE_DO) && (type != DB_TYPE_AO))
        return -1;

    return db_readctrlpoint(DB_CONVENT_ID_2_CHNLNO(ul32ID),
                            DB_CONVENT_ID_2_DEVNO(ul32ID),
                            DB_CONVENT_ID_2_INDEX(ul32ID),
                            f64Value);
}

static int32_t db_writectrlpoint(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value)
{
    struct db_spl_remap_info_t *pIndex;
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_ctrl_tbl_t *pCtrl;
    struct db_spl_realtime_data_node_t *pNode;

    if((pDB == NULL) || (f64Value == NULL))
        return -1;

    pIndex = db_getCtrlIndexByPortAndDev(ucnPort, ucnDev);
    if(!pIndex)
        return -1;

    if(ucnIndex >= pIndex->node_num)
        return -1;

    pCtrl = &pDB->ctrl_node_region[0].context;
    pNode = &pCtrl->node[0];
    pNode = pNode + pIndex->index;
    pNode[ucnIndex].value_now = (int64_t)(*f64Value * 1000);

    return 0;
}

static int32_t db_writectrlpoint_by_id(uint32_t ul32ID, double *f64Value)
{
    uint16_t type = DB_CONVENT_ID_2_TYPE(ul32ID);

    if((type != DB_TYPE_DO) && (type != DB_TYPE_AO))
        return -1;

    return db_writectrlpoint(DB_CONVENT_ID_2_CHNLNO(ul32ID),
                             DB_CONVENT_ID_2_DEVNO(ul32ID),
                             DB_CONVENT_ID_2_INDEX(ul32ID),
                             f64Value);
}



/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }

	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*f64Value = ((double)pNode[ucnIndex].value_now) / 1000;

    return 0;
}

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_by_id(uint32_t ul32ID, double *f64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }

	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*f64Value = ((double)pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now) / 1000;

    return 0;
}

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, double *f64Value)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址
    
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }

	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*f64Value = ((double)pNode[ucnIndex].value_now) / 1000;

    return 0;
}



/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, double *f64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[ucnIndex].value_now = (int64_t)(*f64Value * 1000);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_by_id(uint32_t ul32ID, double *f64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now = (int64_t)(*f64Value * 1000);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : f64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, double *f64Value)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[ucnIndex].value_now = (int64_t)(*f64Value * 1000);

    return 0;
}




//-----------------------操作long long类型读写点----------------------------------------------------------------------------------------

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值 整型，用于存放如IP， MAC等整型数据
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_int64(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, int64_t *i64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*i64Value = (pNode[ucnIndex].value_now);

    return 0;
}


/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_int64_by_id(uint32_t ul32ID, int64_t *i64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*i64Value = (pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now);

    return 0;
}

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_int64_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, int64_t *i64Value)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	*i64Value = (pNode[ucnIndex].value_now);

    return 0;
}



/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_int64(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, int64_t *i64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }

	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[ucnIndex].value_now = (int64_t)(*i64Value);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_int64_by_id(uint32_t ul32ID, int64_t *i64Value)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now = (int64_t)(*i64Value);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_int64_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, int64_t *i64Value)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	pNode[ucnIndex].value_now = (int64_t)(*i64Value);

    return 0;
}




//-----------------------操作string类型读写点----------------------------------------------------------------------------------------

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值 整型，用于存放如IP， MAC等整型数据
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_string(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, char *str, uint8_t str_len)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(str, &pNode[ucnIndex].value_now, str_len);

    return 0;
}

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_string_by_id(uint32_t ul32ID, char *str, uint8_t str_len)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址

	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(str, &pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now, str_len);

    return 0;
}

/*******************************************************************************
* Function Name  : ReadPoint
* Description    : 读取指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_readpoint_string_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, char *str, uint8_t str_len)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode ;  // 节点表首地址
	
	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(str, &pNode[ucnIndex].value_now, str_len);

    return 0;
}



/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_string(uint8_t ucnPort, uint8_t ucnDev, uint16_t ucnIndex, char *str, uint8_t str_len)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	pIndex = db_getIndexByPortAndDev(ucnPort, ucnDev);
	if(!pIndex)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }

	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(&pNode[ucnIndex].value_now, str, str_len);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_string_by_id(uint32_t ul32ID, char *str, uint8_t str_len)
{
	struct db_spl_remap_info_t *pIndex;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	pIndex = db_getIndexByPortAndDev(DB_CONVENT_ID_2_CHNLNO(ul32ID) , DB_CONVENT_ID_2_DEVNO(ul32ID));
	if(!pIndex)
		return -1;
	
	if(DB_CONVENT_ID_2_INDEX(ul32ID) >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;

	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(&pNode[DB_CONVENT_ID_2_INDEX(ul32ID)].value_now, str, str_len);

    return 0;
}

/*******************************************************************************
* Function Name  : WritePoint
* Description    : 写入指定点的实时值
*                  
* Input          : ucnPort: 端口号，0，1，2,外面序号从1开始,实现上需转换一下
*                  ucnDev:  设备号,外面序号从1开始,实现上需转换一下
*                  ucnIndex: 测点序号
* Output         : i64Value: 测点的实时值
* Return         : 0: success   -1: fail
*******************************************************************************/
static int32_t db_writepoint_string_in1dev(struct db_spl_remap_info_t *pIndex, uint16_t ucnIndex, char *str, uint8_t str_len)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	struct db_spl_realtime_data_node_t *pNode;  // 节点表首地址

	/*
		str_len % 8 == 0
		sizeof(int64) 倍数
	*/
	if((str_len & 0x7) != 0)
		return -1;
	
	if(ucnIndex >= pIndex->node_num)
		return -1;

    // 兼容设计
    if(pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        pNode  = &pDB->node;
    }
    else 
    {
        pNode = &pDB256->node;
    }
	/* 指向该设备首地址 */
	pNode = pNode + pIndex->index;
	/*
		we storge the data with int64_t.   
		because it insert into sqlite3 with INTER. it use less ROM than DOUBLE
	*/
	memcpy(&pNode[ucnIndex].value_now, str, str_len);

    return 0;
}



/*********************************************************************************************************
 ** Function name:       mb_ctrl_idle_queue
 ** Descriptions:        在一段连续控制槽中查找第一条 is_active 为 false 的空闲槽。
 ** input parameters:      phead   控制槽数组首地址；
 **                        tbl_len 本段槽位个数。
 ** output parameters:     NONE
 ** Returned value:         空闲槽指针；无空闲返回 NULL。
 *********************************************************************************************************/
static struct db_spl_channel_ctr_t *mb_ctrl_idle_queue(struct db_spl_channel_ctr_t *phead, uint8_t tbl_len)
{
    int i;

    for(i=0; i<tbl_len; i++)
    {
        if(phead[i].is_active == false)
            return &phead[i];
    }

    return NULL;
}

/*********************************************************************************************************
 ** Function name:       mb_ctrl_first_active_segment
 ** Descriptions:        在一段连续控制槽数组中查找第一条 is_active 非 0 的项（用于主池或扩展池单段扫描）。
 ** input parameters:      phead  控制槽数组首地址；
 **                        tbl_len 本段槽位个数。
 ** output parameters:     NONE
 ** Returned value:       指向第一条待下发项的指针；无则返回 NULL。
 *********************************************************************************************************/
static struct db_spl_channel_ctr_t *mb_ctrl_first_active_segment(struct db_spl_channel_ctr_t *phead, uint8_t tbl_len)
{
    int i;

    for (i = 0; i < tbl_len; i++)
    {
        if (phead[i].is_active)
            return &phead[i];
    }
    return NULL;
}

/*********************************************************************************************************
 ** Function name:       db_ctrl_alloc_idle
 ** Descriptions:        为指定通道分配一条空闲控制命令槽：优先使用主池 ctrl[][]，满后再使用
 **                      ctrl_extend_region.extend.context[][] 扩展池（与主池非物理连续，逻辑上为二级池）。
 ** input parameters:      pDB     实时共享内存表指针（兼容 db_spl_realtime_data_tbl_t / _256_t）；
 **                        chnl_id 通道号（0 .. DB_CFG_MAX_PORT-1）。
 ** output parameters:     NONE
 ** Returned value:         可用槽指针；参数非法或主池与扩展池均无空槽时返回 NULL。
 *********************************************************************************************************/
struct db_spl_channel_ctr_t *db_ctrl_alloc_idle(struct db_spl_realtime_data_tbl_t *pDB, uint8_t chnl_id)
{
    struct db_spl_channel_ctr_t *p;
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

    if (!pDB || chnl_id >= DB_CFG_MAX_PORT)
        return NULL;

    if (pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        p = mb_ctrl_idle_queue(&pDB->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
        if (p)
            return p;
        return mb_ctrl_idle_queue(&pDB->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
    }

    p = mb_ctrl_idle_queue(&pDB256->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
    if (p)
        return p;
    return mb_ctrl_idle_queue(&pDB256->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
}

/*********************************************************************************************************
 ** Function name:       db_ctrl_active_first
 ** Descriptions:        获取本通道下一条待下发的控制项：先扫描主池，主池无待下发项时再扫描扩展池，
 **                      保证扩展池中的命令在主池清空后也会被下发。
 ** input parameters:      pDB     实时共享内存表指针；
 **                        chnl_id 通道号（0 .. DB_CFG_MAX_PORT-1）。
 ** output parameters:     NONE
 ** Returned value:         待下发项指针；主池与扩展池均无 is_active 时返回 NULL。
 *********************************************************************************************************/
struct db_spl_channel_ctr_t *db_ctrl_active_first(struct db_spl_realtime_data_tbl_t *pDB, uint8_t chnl_id)
{
    struct db_spl_channel_ctr_t *p;
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

    if (!pDB || chnl_id >= DB_CFG_MAX_PORT)
        return NULL;

    if (pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
    {
        p = mb_ctrl_first_active_segment(&pDB->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
        if (p)
            return p;
        return mb_ctrl_first_active_segment(&pDB->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
    }

    p = mb_ctrl_first_active_segment(&pDB256->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
    if (p)
        return p;
    return mb_ctrl_first_active_segment(&pDB256->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
}


/*********************************************************************************************************
 ** Function name:		 
** Descriptions:		 
** input parameters:			  
** output parameters:	 NONE
** Returned value: 	 
*********************************************************************************************************/
static void mb_ctrl_queue_flush(struct db_spl_channel_ctr_t *phead, uint8_t tbl_len)
{
    int i;
    
    for(i=0; i<tbl_len; i++)
    {
        phead[i].is_active = false;
    }
}

/*********************************************************************************************************
 ** Function name:       mb_ctrl_queue_pending_count
 ** Descriptions:        统计一段连续控制槽中 is_active 非 0 的条数（单段，主池或扩展池之一）。
 ** input parameters:      phead   控制槽数组首地址；
 **                        tbl_len 本段槽位个数。
 ** output parameters:     NONE
 ** Returned value:       待下发条数。
 *********************************************************************************************************/
static int mb_ctrl_queue_pending_count(struct db_spl_channel_ctr_t *phead, uint8_t tbl_len)
{
    int i, n = 0;
    for (i = 0; i < tbl_len; i++)
        if (phead[i].is_active)
            n++;
    return n;
}

/*********************************************************************************************************
 ** Function name:       db_command_op_write
 ** Descriptions:        上层写入一条控制命令到实时库：按 nodeid 解析通道并占用空闲控制槽（含扩展池）。
 ** input parameters:      nodeid  测点/控制点 ID；
 **                        f64val  下发数值。
 ** output parameters:     NONE
 ** Returned value:         0 成功；-1 无空闲槽或其它失败。
 *********************************************************************************************************/
static int db_command_op_write(uint32_t nodeid, double f64val)
{
#define _FC_WRITE_MULTIPLE_REGISTERS 0x10

	uint8_t                           chnl_id;
    struct db_spl_channel_ctr_t       *pctrl;
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);

	// 获取通道号
	chnl_id = DB_CONVENT_ID_2_CHNLNO(nodeid);

    pctrl = db_ctrl_alloc_idle(pDB, chnl_id);
    if(pctrl == NULL)
	{
		printf("db_command_op_write: nodeid:%X, f64val:%lf, pctrl is NULL\n", nodeid, f64val);
		return -1;
	}

    pctrl->funcode = _FC_WRITE_MULTIPLE_REGISTERS;

    pctrl->is_active = true;
    pctrl->db_linkid = nodeid;
    pctrl->value     = f64val;


    return 0;
}


/*********************************************************************************************************
 ** Function name:       db_command_op_flush
 ** Descriptions:        清空指定通道全部待下发控制项（主池 ctrl 与扩展池 extend.context 一并清除）。
 ** input parameters:      chnl_id 通道号。
 ** output parameters:     NONE
 ** Returned value:         0 成功；-1 参数无效。
 *********************************************************************************************************/
int db_command_op_flush(uint8_t chnl_id)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;

	if (!pDB || chnl_id >= DB_CFG_MAX_PORT)
		return -1;

	if (pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
	{
		mb_ctrl_queue_flush(&pDB->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
		mb_ctrl_queue_flush(&pDB->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
	}
	else
	{
		mb_ctrl_queue_flush(&pDB256->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
		mb_ctrl_queue_flush(&pDB256->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
	}

    return 0;
}

/*********************************************************************************************************
 ** Function name:       db_ctrl_queue_pending_count
 ** Descriptions:        返回指定通道主池与扩展池中尚未下发控制条数之和（供防逆流等逻辑等待队列清空）。
 ** input parameters:      chnl_id 通道号。
 ** output parameters:     NONE
 ** Returned value:         待下发条数；表指针无效时返回 0。
 ** Notice:                与 db_node_fops.h 中声明一致。
 *********************************************************************************************************/
int db_ctrl_queue_pending_count(uint8_t chnl_id)
{
	struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
	struct db_spl_realtime_data_tbl_256_t *pDB256 = (struct db_spl_realtime_data_tbl_256_t *)pDB;
	int n;

	if (!pDB || chnl_id >= DB_CFG_MAX_PORT)
		return 0;
	if (pDB->device_num <= DB_CFG_MAX_DEV_COMPATIBLE)
	{
		n = mb_ctrl_queue_pending_count(&pDB->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
		n += mb_ctrl_queue_pending_count(&pDB->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
	}
	else
	{
		n = mb_ctrl_queue_pending_count(&pDB256->ctrl[chnl_id][0], DB_CFG_MAX_CTRL_QUEUE);
		n += mb_ctrl_queue_pending_count(&pDB256->ctrl_extend_region.extend.context[chnl_id][0], DB_CFG_CTRL_EXTEND_SLOTS);
	}
	return n;
}

/*******************************************************************************
* api Name  	 : DataMgmtFops
* Description    : 实时数据库操作接口API
*                  
*******************************************************************************/
const struct DataMgmt_fops DataMgmtFops = {
	.init					= db_init,                                          
	.getIndexByPortAndDev   = db_getIndexByPortAndDev,	
    .getIndexDevBySeq       = db_getIndexDevBySeq,                                        
	.readpoint              = db_readpoint,
	.readpoint_by_id		= db_readpoint_by_id,
	.readpoint_in1dev       = db_readpoint_in1dev,
	.writepoint             = db_writepoint,
	.writepoint_by_id		= db_writepoint_by_id,
	.writepoint_in1dev      = db_writepoint_in1dev,

    .readpoint_int64        = db_readpoint_int64,
	.readpoint_int64_by_id	= db_readpoint_int64_by_id,
	.readpoint_int64_in1dev = db_readpoint_int64_in1dev,
	.writepoint_int64       = db_writepoint_int64,
	.writepoint_int64_by_id	= db_writepoint_int64_by_id,
	.writepoint_int64_in1dev= db_writepoint_int64_in1dev,

    .readpoint_string       = db_readpoint_string,
	.readpoint_string_by_id	= db_readpoint_string_by_id,
	.readpoint_string_in1dev= db_readpoint_string_in1dev,
	.writepoint_string        = db_writepoint_string,
	.writepoint_string_by_id  = db_writepoint_string_by_id,
	.writepoint_string_in1dev = db_writepoint_string_in1dev,

    .map_node_num             = db_map_node_num,
    .map_dev_num              = db_map_dev_num,
    .map_active_node_size     = db_map_active_node_size,

	.write_command_op	      = db_command_op_write,
	.flush_command_op         = db_command_op_flush,

    .readctrlpoint           = db_readctrlpoint,
    .readctrlpoint_by_id     = db_readctrlpoint_by_id,
    .writectrlpoint          = db_writectrlpoint,
    .writectrlpoint_by_id    = db_writectrlpoint_by_id,
};


/*************************************************************************/
/** Function Name:       db_debug_region                                 **/
/** Purpose:             取表尾调试域（64/256 布局绝对偏移不同）          **/
/*************************************************************************/
struct db_spl_debug_region_t *db_debug_region(struct db_spl_realtime_data_tbl_t *pDB)
{
	struct db_spl_debug_region_t *pdbg;

	if (pDB == NULL)
		return NULL;

	if (pDB->device_num <= 64)
		pdbg = &pDB->debug;
	else
		pdbg = &((struct db_spl_realtime_data_tbl_256_t *)pDB)->debug;

	if (pdbg->magic != DB_DEBUG_MAGIC || pdbg->version != DB_DEBUG_LAYOUT_VERSION ||
	    pdbg->size != (uint32_t)sizeof(struct db_spl_debug_region_t))
	{
		memset(pdbg, 0, sizeof(*pdbg));
		pdbg->magic = DB_DEBUG_MAGIC;
		pdbg->version = DB_DEBUG_LAYOUT_VERSION;
		pdbg->size = (uint32_t)sizeof(struct db_spl_debug_region_t);
		pdbg->bypass.chnl = 0;
		pdbg->bypass.status = DB_BYPASS_STATUS_IDLE;
	}

	return pdbg;
}


/*************************************************************************/
/** Function Name:       db_bypass_ctl                                   **/
/*************************************************************************/
struct db_spl_bypass_ctl_t *db_bypass_ctl(struct db_spl_realtime_data_tbl_t *pDB)
{
	struct db_spl_debug_region_t *pdbg = db_debug_region(pDB);

	if (pdbg == NULL)
		return NULL;
	return &pdbg->bypass;
}
