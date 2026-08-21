#include "edge_point_write.h"

#include "edge_point_util.h"
#include "db_node_fops.h"
#include "db_struct.h"

#include <unistd.h>

#define EDGE_CTRL_WRITE_WAIT_MS   3000U
#define EDGE_CTRL_WRITE_POLL_MS   20U

/*************************************************************************/
/** Function Name:       edge_point_is_virtual_channel                     **/
/** Purpose:             判断通道是否为虚拟/内存写点通道                    **/
/** Params:            chnl_id - 通道号                                    **/
/** Return:            1-虚拟通道；0-物理南向通道                          **/
/** Notice:            193 为系统通道，9-16 为 Modbus 虚拟互联通道         **/
/*************************************************************************/
int edge_point_is_virtual_channel(uint8_t chnl_id)
{
    if (chnl_id == 193)
        return 1;
    if (chnl_id >= 9 && chnl_id <= 16)
        return 1;
    return 0;
}

/*************************************************************************/
/** Function Name:       edge_point_is_memory_write_point                  **/
/** Purpose:             判断是否应直写实时库（不经南向控制队列组包）        **/
/** Params:            id - dblink_id                                      **/
/** Return:            1-内存直写；0-走 write_command_op                   **/
/** Notice:            COM9-16/193 为虚拟互联通道；COM1-8 内南向配置      **/
/**                    funcode/code=0 的 DO/AO 虚拟测点亦仅写实时库      **/
/*************************************************************************/
static int edge_point_is_memory_write_point(uint32_t id)
{
    uint8_t  chnl = edge_dblink_channel(id);
    uint16_t type = DB_CONVENT_ID_2_TYPE(id);

    if (edge_point_is_virtual_channel(chnl))
        return 1;

    return 0;
}

/*************************************************************************/
/** Function Name:       edge_point_write_memory                           **/
/** Purpose:             将测点/控制点值写入实时库                          **/
/** Params:            id - dblink_id；val - 写入值；is_ctrl - 是否控制点  **/
/** Return:            0-成功；-1-失败                                     **/
/*************************************************************************/
static int edge_point_write_memory(uint32_t id, double val, int is_ctrl)
{
    if (is_ctrl && DataMgmtFops.writectrlpoint_by_id)
    {
        if (DataMgmtFops.writectrlpoint_by_id(id, &val) == 0)
            return 0;
    }

    if (DataMgmtFops.writepoint_by_id && DataMgmtFops.writepoint_by_id(id, &val) == 0)
        return 0;

    return -1;
}

/*************************************************************************/
/** Function Name:       edge_point_wait_ctrl_idle                         **/
/** Purpose:             等待指定通道控制队列出现空闲槽                      **/
/** Params:            chnl_id - 通道号                                    **/
/** Return:            0-可写；-1-超时仍无空闲槽                           **/
/** Notice:            避免 write_command_op 因队列满返回 pctrl NULL       **/
/*************************************************************************/
static int edge_point_wait_ctrl_idle(uint8_t chnl_id)
{
    struct db_spl_realtime_data_tbl_t *pDB = db_DataTblObj(NULL);
    unsigned waited = 0;

    if (!pDB)
        return -1;

    while (waited < EDGE_CTRL_WRITE_WAIT_MS)
    {
        if (db_ctrl_alloc_idle(pDB, chnl_id) != NULL)
            return 0;

        usleep(EDGE_CTRL_WRITE_POLL_MS * 1000U);
        waited += EDGE_CTRL_WRITE_POLL_MS;
    }

    return -1;
}

/*************************************************************************/
/** Function Name:       edge_point_read                                   **/
/** Purpose:             按 dblink_id 读取测点或控制点实时值                **/
/** Params:            id - dblink_id；val - 输出值；ctrl_only - 仅读控制点**/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            非 ctrl_only 时先 readpoint，失败再 readctrlpoint     **/
/*************************************************************************/
int edge_point_read(uint32_t id, double *val, int ctrl_only)
{
    if (!val)
        return -1;

    if (!ctrl_only && DataMgmtFops.readpoint_by_id)
    {
        if (DataMgmtFops.readpoint_by_id(id, val) == 0)
            return 0;
    }

    if (DataMgmtFops.readctrlpoint_by_id)
        return DataMgmtFops.readctrlpoint_by_id(id, val);

    return -1;
}

/*************************************************************************/
/** Function Name:       edge_point_write                                  **/
/** Purpose:             按 dblink_id 写测点或控制点，自动路由写方式        **/
/** Params:            id - dblink_id；val - 写入值；is_ctrl - 是否控制点写**/
/** Return:            0-成功；-1-失败                                     **/
/** Notice:            虚拟通道及 COM1-8 内 funcode=0 的 DO/AO 走 RAM 写；    **/
/**                    其余物理测点走 write_command_op                       **/
/*************************************************************************/
int edge_point_write(uint32_t id, double val, int is_ctrl)
{
    if (edge_point_is_memory_write_point(id))
        return edge_point_write_memory(id, val, is_ctrl);

    if (!DataMgmtFops.write_command_op)
        return -1;

    if (edge_point_wait_ctrl_idle(edge_dblink_channel(id)) != 0)
        return -1;

    if (DataMgmtFops.write_command_op(id, val) != 0)
        return -1;

    return 0;
}
