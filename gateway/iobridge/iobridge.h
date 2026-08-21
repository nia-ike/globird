/*
* Copyright (c) 
* All rights reserved.
*
* 文件名称：
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
*  chenzy   2020.6.09   V1.00      创建
*/



#ifndef __IOBRIDGE_H__
#define __IOBRIDGE_H__

#include <stdint.h>
#include "hal_comm/hal_com_baseclass.h"

struct iobridge_ctx_t
{;
    /* 北向通信 */
    struct hal_comm_ctx_t northCommCtx;
    /* 南向通信 */
    struct hal_comm_ctx_t southCommCtx;

};






int iobridge_init(struct iobridge_ctx_t *pSer2netCtx);

#endif