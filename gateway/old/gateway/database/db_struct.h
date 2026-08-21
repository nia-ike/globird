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

#ifndef __DB_STRUCT_H__
#define __DB_STRUCT_H__

#include <stdint.h>

/* 
	data attribute setting macro
	强烈注意： 当前版本这里的宏定义必须与datamgmt文件的数量宏定义一致
*/
#define DB_CFG_MAX_DEV_COMPATIBLE		    64			/* 最大支持设备数, 兼容旧设备用途 */
#define DB_CFG_MAX_DEV						256			/* 最大支持设备数 */

#define DB_CFG_MAX_PORT						64			/* 电表类通道数量 rs485(1) + NET + can */
#define DB_CFG_MAX_POINT					256			/* 单台智能设备最大测点数 */

#define DB_CFG_NODE_NAME_LEN				16			/* 测点名称长度 */
#define DB_CFG_NODE_UNIT_LEN				8			/* 单位最大长度 */
#define DB_CFG_MAX_REQUEST_PKT_LEN			16
#define DB_CFG_MAX_CTRLNODESIZE_PER_DEVICE	8
#define DB_CFG_MAX_ALARMRULE_PER_DEVICE     16          /* 单台智能设备最大告警规则数 */

#define DB_CFG_MAX_CTRL_QUEUE				32
/* 扩展控制池槽位数：与 db_spl_channel_ctr_extend_t.context 每行长度一致（DB_CFG_MAX_CTRL_QUEUE * 2） */
#define DB_CFG_CTRL_EXTEND_SLOTS			(DB_CFG_MAX_CTRL_QUEUE * 2)
/* 控制队列 FIFO 深度 = 主池槽数 + 扩展池槽数（与每通道最大在途条数一致） */
#define DB_CFG_CTRL_QUEUE_FIFO_DEPTH		(DB_CFG_MAX_CTRL_QUEUE + DB_CFG_CTRL_EXTEND_SLOTS)
/* 每通道物理槽占用位图字数：ceil(FIFO_DEPTH / 32)，与 pool_occupied[] 一致 */
#define DB_CFG_CTRL_POOL_OCCUPIED_WORDS		((DB_CFG_CTRL_QUEUE_FIFO_DEPTH + 31u) / 32u)
/*
 * 扩展 union 内 index 管理区总字节数：顶满后 sizeof(db_spl_channel_ctr_extend_t) 与 pad[8192] 对齐（131072）。
 */
#define DB_CFG_CTRL_INDEX_TABLE_BYTES		32768
/* 管理表魔数与布局版本：首次初始化或迁移时写入 index.magic / index.layout_version */
#define DB_CTRL_QIDX_MAGIC			0x4C525443u	/* 小端内存序 "CTRL" */
#define DB_CTRL_QIDX_LAYOUT_VERSION		1u

#define DB_CFG_MAX_DATA_NODE_NUM		    8192        /* 原设计，设计最大采集点数量 8192个槽位 */
#define DB_CFG_MAX_CTRL_NODE_NUM		    4096        /* 新增，设计最大控制点数量 4096个槽位 */

#define FRAME_RECVBUF_SIZE	    1460       /* */

enum 
{
	DB_TYPE_AI = 0x0000,
	DB_TYPE_DI = 0x0100,
	DB_TYPE_ACC = 0x0200,
	DB_TYPE_DO = 0x0300,
	DB_TYPE_AO  = 0x0400
};



//------------------------------------------------------------------
//      设备ID 组成：
//                  PORT   DEV       index
//                   1B     1B    2B（0-65536
//------------------------------------------------------------------
#define DB_CONVENT2ID(chnl, dev, type, id)		((chnl << 24) | (dev << 16) | (type << 8) | ((id & 0xf00)<< 12) | (id &0xff) )		
#define DB_CONVENT_CHNLDEV_GROUP(chnl, dev) (((uint16_t)chnl << 8) | dev)

#define DB_CONVENT_ID_2_CHNLNO(id)			((id >> 24) & 0xff)
#define DB_CONVENT_ID_2_DEVNO(id)			((id >> 16) & 0xff)
#define DB_CONVENT_ID_2_INDEX(id)			(((id & 0x0000f000) >> 4) | (id & 0xff))//      ((id) & 0xff)    // 扩展单个设备超过256个点,  沿用 XG方式， 第二字节的低4位位类型

#define DB_CONVENT_ID_2_TYPE(id)			((id & 0xf00))  // 不能修改成 >>8，因为so 采集驱动已经按这个9-12bit 运行了


/*
	REALTIME database mtd:

    ___________________
	|	 有效设备数		|
	___________________
	|					|
	|     index         |              [16 x indexs]
	| _________________ |
	|     dev 1         |        <-- pDevNodeHead
	|     dev 2         |              [node buffer 16 x 128 nodes] 
	|     dev 3         |
	|	  ...           |	
    ---------------------

	===========================================

	index :  
		id : 				channel NO. and device NO.
		

	method:
		for(i=0; i<DM_CFG_MAX_DEV; i++)
		{
			if(index[i].id.mix == 0x0102)
			{
				return index[i].pDevNodeHead;
			}
		}
		

*/

/*
	db index'id define.
	for example:  channel no: 01   device no:02

	we can search by id.mix [0x0102], also can search by id.split.chnlNo [01] and id.split.devNo [02] 

	notice that:  id = 0 mean that this index is null
*/


struct db_spl_remap_info_t
{
	// ================== just type define, only use by (struct db_spl_remap_info_t) 
	union db_node_index_id 
	{
		struct db_node_index_id_split
		{

			/* little endian CPU,  byte order is (dev -> port), uint16_t  high byte is port , low byte is dev, so we  define dev first */
			uint8_t			ucnDev;
			uint8_t			ucnPort;
		} split;

		uint16_t						mix;
	} id;
	// ==============================================


	/* the device be mapping have [node_num] nodes */
	uint16_t							node_num;	// 当前设备的节点数量
    
	uint32_t							index;		// 当前设备的首节点在node表的索引号  node[index] 即为当前设备的第一个节点地址
};


struct db_spl_realtime_data_node_t
{
    uint64_t 		flag;       	//	revered
    int64_t 		value_now;      //  value.   int64_t ,  x 1000, for example 220.12V => 220120
};



/*
控制测点池 DO/AO 的结构体
*/
struct db_spl_ctrl_tbl_t
{
    uint8_t device_num;
	uint16_t node_num;
	struct db_spl_remap_info_t index[DB_CFG_MAX_DEV];
	struct db_spl_realtime_data_node_t node[DB_CFG_MAX_CTRL_NODE_NUM];
};


/*
    控制测点池 DO/AO 的联合体
    用于控制测点池 DO/AO 的联合体
    context 用于控制测点池 DO/AO 的结构体
    pad 用于控制测点池 DO/AO 的预留

	以前设计时，控制点只会直接转成控制队列，不写入点值。
	现在需要将控制点值写入到控制测点池中，同时也写入到控制队列。
	由于 db_spl_realtime_data_tbl_t 已经接近占完1MB的内存，
	而原来node 和 node_space 已经占用了 8192 * 6 个槽位（实际上只用到8192个槽位，剩余5个是预留的），
    修改为  8192 * 3 给采集点
	提1个region区域给控制点，共 8192 * 1 槽位内存用于db_spl_ctrl_region_t

*/
union db_spl_ctrl_node_region_t 
{
	struct db_spl_ctrl_tbl_t           context;     // 实际控制点的结构体。注意本结构体大小要小于等于 下面的pad[] 的大小
	struct db_spl_realtime_data_node_t pad[DB_CFG_MAX_DATA_NODE_NUM];  // 占位。 用于占用8192个槽位
};

struct db_spl_channel_ctr_t 
{
    uint32_t            is_active;
    uint32_t            db_linkid;
    uint8_t             funcode;
	uint8_t             reserved[7];
    double              value;

};

/*
 * 每端口一条 FIFO 逻辑队列：元素为 slot_ref（uint8_t）
 *   0 .. DB_CFG_MAX_CTRL_QUEUE-1               → ctrl[port][slot_ref]
 *   DB_CFG_MAX_CTRL_QUEUE .. FIFO_DEPTH-1      → extend.context[port][slot_ref - DB_CFG_MAX_CTRL_QUEUE]
 *
 * count：FIFO 中待下发条数。消费者侧若仅判断「本通道是否有排队命令」，先看 count==0 即可 O(1) 返回，
 *       不必每周期扫 ring[96]。
 *
 * pool_occupied[0..2]：96 个物理槽的占用位图（bit i 对应 slot_ref i；1=该槽已挂命令、与 ctrl 池一致）。
 *       分配空槽时可对最多 3 个 uint32_t 做 ffs/扫描找 0 位，通常比连扫 96 个 is_active 更省；
 *       亦可 (w0|w1|w2)==0 作「本通道池全空」的快速过滤（与 count==0、环空在一致维护下应对齐）。
 *       不变量建议：popcount(三词) == count，且与 slot_ring 中各 slot_ref 互相对应。
 *
 * 与仅使用 ctrl[][] 主池（查空即写、不维护本表）的旧进程共存时：
 *   - 分配 slot_ref 时应优先使用 0 .. DB_CFG_MAX_CTRL_QUEUE-1，主池用满后再用扩展槽；这样在待下发条数
 *     始终 < 32 时，命令只落在原 ctrl[port][] 物理区，与旧代码「只在主池填队列」的落点一致。
 *   - 但若旧进程只写 is_active 等、从不更新本 lane 的 ring/count/位图，则仅按 FIFO 元数据消费的逻辑
 *     会「看不到」这些条目，需另做兼容（例如消费端保留对主池/扩展池的兜底扫描，或统一升级写端）。
 *   - 旧逻辑不保证先后顺序；新 FIFO 保证顺序。混用不同消费策略时，下发顺序可能仍不一致。
 *
 * 跨进程一致性由业务侧自行保证（本表不含锁字段）。
 */
struct db_spl_channel_ctr_fifo_lane_t
{
	uint8_t			slot_ring[DB_CFG_CTRL_QUEUE_FIFO_DEPTH];
	uint8_t			head;
	uint8_t			tail;
	uint8_t			count;
	uint8_t			_pad_lane[5];
	uint32_t		pool_occupied[DB_CFG_CTRL_POOL_OCCUPIED_WORDS];
};

#define DB_SPL_CTRL_QIDX_HDR_BYTES		64
/* magic(4)+layout_version(4)+max_port(2)+fifo_depth(2)+flags(4)+generation(4) */
#define DB_SPL_CTRL_QIDX_HDR_USED		20
#define DB_SPL_CTRL_QIDX_LANE_STRIDE	sizeof(struct db_spl_channel_ctr_fifo_lane_t)
#define DB_SPL_CTRL_QIDX_RESERVED_TAIL	(DB_CFG_CTRL_INDEX_TABLE_BYTES - DB_SPL_CTRL_QIDX_HDR_BYTES \
						- DB_SPL_CTRL_QIDX_LANE_STRIDE * DB_CFG_MAX_PORT)

/*
 * 共享内存内控制队列管理表（主池 + 扩展池合一 FIFO 元数据）
 * 总大小固定为 DB_CFG_CTRL_INDEX_TABLE_BYTES（当前 32KiB），尾部 reserved_extension 供后续统计/恢复等扩展。
 */
struct db_spl_channel_ctr_index_table_t
{
	uint32_t		magic;
	uint32_t		layout_version;
	uint16_t		max_port;
	uint16_t		fifo_depth;
	uint32_t		flags;
	uint32_t		generation;
	uint8_t			_hdr_reserved[DB_SPL_CTRL_QIDX_HDR_BYTES - DB_SPL_CTRL_QIDX_HDR_USED];

	struct db_spl_channel_ctr_fifo_lane_t	lane[DB_CFG_MAX_PORT];

	uint8_t			reserved_extension[DB_SPL_CTRL_QIDX_RESERVED_TAIL];
};

// 控制队列的扩展区域
struct db_spl_channel_ctr_extend_t
{
	struct db_spl_channel_ctr_index_table_t index;
	struct db_spl_channel_ctr_t context[DB_CFG_MAX_PORT][DB_CFG_MAX_CTRL_QUEUE * 2];
};

/*
    控制队列的扩展区域（与 pad[] 共用 8192 个 db_spl_realtime_data_node_t 槽位）
    extend：index_table + 每端口 context[DB_CFG_MAX_CTRL_QUEUE*2] 控制槽
*/
union db_spl_channel_ctr_extend_region_t
{
	struct db_spl_channel_ctr_extend_t extend;
	struct db_spl_realtime_data_node_t pad[DB_CFG_MAX_DATA_NODE_NUM];
};

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
_Static_assert(DB_CFG_CTRL_QUEUE_FIFO_DEPTH == 96, "FIFO depth must match primary+extend slots");
_Static_assert(DB_CFG_CTRL_POOL_OCCUPIED_WORDS == 3u && DB_CFG_CTRL_POOL_OCCUPIED_WORDS * 32u >= DB_CFG_CTRL_QUEUE_FIFO_DEPTH,
		"pool_occupied must cover all physical slots");
_Static_assert(sizeof(struct db_spl_channel_ctr_index_table_t) == DB_CFG_CTRL_INDEX_TABLE_BYTES,
		"db_spl_channel_ctr_index_table_t must fill DB_CFG_CTRL_INDEX_TABLE_BYTES");
_Static_assert(sizeof(union db_spl_channel_ctr_extend_region_t)
		== sizeof(struct db_spl_realtime_data_node_t) * DB_CFG_MAX_DATA_NODE_NUM,
		"ctrl_extend_region must match pad[8192] slot footprint");
#endif


struct db_spl_channel_trans_t
{
    uint32_t            is_active;
	uint32_t            timeout;
    uint8_t             sendbuf[FRAME_RECVBUF_SIZE];
	uint32_t            sendlen;
	uint8_t             recvbuf[FRAME_RECVBUF_SIZE];
	uint32_t            recvlen;
	uint8_t             reserved[32];
};

/* ------------------------------------------------------------------ */
/* 调试域（表尾追加）：临时串口透传等，不占用中间 pad                 */
/* ------------------------------------------------------------------ */
#define DB_DEBUG_MAGIC				0x44424744u	/* 小端 "DBGD" */
#define DB_DEBUG_LAYOUT_VERSION		1u
#define DB_DEBUG_RESERVED_BYTES		256

#define DB_BYPASS_SERIAL_CHNL_MIN	1
#define DB_BYPASS_SERIAL_CHNL_MAX	8		/* 仅物理串口 COM1~8 */
#define DB_BYPASS_LEASE_SEC_DEFAULT	600
#define DB_BYPASS_LEASE_SEC_MAX		3600
#define DB_BYPASS_PORT_BASE			7000	/* 默认 listen = 7000 + chnl */

#define DB_BYPASS_STATUS_IDLE		0
#define DB_BYPASS_STATUS_LISTEN		1		/* 已监听，等待/已有 TCP 客户端 */
#define DB_BYPASS_STATUS_ACTIVE		2		/* 旁路泵运行中 */
#define DB_BYPASS_STATUS_FAIL		3

/*
 * 全局仅一路临时透传：chnl=0 表示关闭；各 sampling 比对自身 ChannelNo。
 * 北向仅 TCP Server；南向复用已打开的串口 FD。
 */
struct db_spl_bypass_ctl_t
{
	int32_t				chnl;			/* 0=关；1..8=旁路该串口通道 */
	uint16_t			listen_port;
	uint16_t			_pad0;
	uint32_t			deadline;		/* unix 秒；到期等价关闭 */
	uint32_t			status;			/* sampling 回写 */
	uint32_t			seq;			/* 每次开/关 +1 */
};

struct db_spl_debug_region_t
{
	uint32_t					magic;
	uint32_t					version;
	uint32_t					size;			/* sizeof(db_spl_debug_region_t) */
	uint32_t					_reserved0;

	struct db_spl_bypass_ctl_t	bypass;

	uint8_t						reserved[DB_DEBUG_RESERVED_BYTES];
};


// old version compatibility.  index size is max 64 devices. 
/*

原定义的结构体； 后续任何修改都必须与这个结构体兼容
struct db_spl_realtime_data_tbl_t
{
    uint8_t                             device_num;                                 // 有效设备数
	uint16_t 							node_num;
    // 兼容旧设备索引。由于之前只支持64个设备。在扩展到128或以上的时候 索引容量不够，本结构体保持旧的64个 设备索引表结构。程序中如遇到超过64台，即使用 db_spl_realtime_data_tbl_256_t 指针强转覆盖
	struct db_spl_remap_info_t			index[DB_CFG_MAX_DEV_COMPATIBLE];                      
	struct db_spl_realtime_data_node_t	node;
    struct db_spl_realtime_data_node_t	node_space[8192 * 6 - 1];                     // 当前RAM 留了1MB， 测点设计师8192. 现在留 8192 * 6 空间作为将来测点扩展  ， 

    // 以上共占用 786952 Bytes 
    // 以下为临时结构体，未固定
    struct db_spl_channel_ctr_t         ctrl[DB_CFG_MAX_PORT][DB_CFG_MAX_CTRL_QUEUE];

	struct db_spl_channel_trans_t       trans[DB_CFG_MAX_PORT];
};

struct db_spl_realtime_data_tbl_256_t
{
    uint8_t                             device_num;                                 // 有效设备数
	uint16_t 							node_num;
    
    // 超出64的设备索引会存放到index[64-254]中，
	struct db_spl_remap_info_t			index[DB_CFG_MAX_DEV];                      
	struct db_spl_realtime_data_node_t	node;
    struct db_spl_realtime_data_node_t	node_space[8192 * 6 - 1];                     // 当前RAM 留了1MB， 测点设计师8192. 现在留 8192 * 6 空间作为将来测点扩展  ， 

    // 以上共占用 786952 Bytes 
    // 以下为临时结构体，未固定
    struct db_spl_channel_ctr_t         ctrl[DB_CFG_MAX_PORT][DB_CFG_MAX_CTRL_QUEUE];

	struct db_spl_channel_trans_t       trans[DB_CFG_MAX_PORT];
};
*/



/*
    we do the compatibe with 256 devices.

    if device_num <= 64,  just USE "struct db_spl_realtime_data_tbl_t"
    if device_num > 64 & device_num < 256, we use the "struct db_spl_realtime_data_tbl_256_t *"  redefine the vmem.

    this will compatibe with old 64 version, and will DO the expand for 256 devices max.

    ------------------------------------------------------------------------
    db_spl_realtime_data_node_t node[8192 * 6]  
	拆成 
	------------------------------------------------------------------------
    db_spl_realtime_data_node_t node[8192 * 3]
    ------------------------------------------------------------------------
    db_spl_ctrl_node_region_t ctrl_region ctrl_node[1]   => 大小 = db_spl_realtime_data_node_t  pad[8192]
	------------------------------------------------------------------------
    db_spl_realtime_data_node_t  pad[8192 * 2]
    ------------------------------------------------------------------------


*/
struct db_spl_realtime_data_tbl_t
{
    uint8_t                             device_num;                                 // 有效设备数
	uint16_t 							node_num;
    // 兼容旧设备索引。由于之前只支持64个设备。在扩展到128或以上的时候 索引容量不够，本结构体保持旧的64个 设备索引表结构。程序中如遇到超过64台，即使用 db_spl_realtime_data_tbl_256_t 指针强转覆盖
	struct db_spl_remap_info_t			index[DB_CFG_MAX_DEV_COMPATIBLE];                      
	struct db_spl_realtime_data_node_t	node;
    struct db_spl_realtime_data_node_t	node_space[DB_CFG_MAX_DATA_NODE_NUM * 3 - 1];                     // 当前RAM 留了1MB， 测点设计师8192. 现在留 8192 * 6 空间作为将来测点扩展. 20260321 修改为 8192 * 3 给采集点

    // 拆分1个region区域给控制点，共 8192 * 1 槽位内存用于db_spl_ctrl_region_t
	union  db_spl_ctrl_node_region_t         ctrl_node_region[1];

	// 剩余2个中的其中1个区域给预留
	struct db_spl_realtime_data_node_t  pad[DB_CFG_MAX_DATA_NODE_NUM];
    // 剩余2个中的其中1个区域给控制队列扩展。  原来设计的 ctrl队列太短了。 因此需要扩展。
    union db_spl_channel_ctr_extend_region_t ctrl_extend_region;
    // 以上共占用 786952 Bytes 
    // 以下为临时结构体，未固定
    struct db_spl_channel_ctr_t         ctrl[DB_CFG_MAX_PORT][DB_CFG_MAX_CTRL_QUEUE];

	struct db_spl_channel_trans_t       trans[DB_CFG_MAX_PORT];

	/* 表尾调试域（1MB vmem 足够；勿占用中间 pad） */
	struct db_spl_debug_region_t        debug;
};


struct db_spl_realtime_data_tbl_256_t
{
    uint8_t                             device_num;                                 // 有效设备数
	uint16_t 							node_num;
    
    // 超出64的设备索引会存放到index[64-254]中，
	struct db_spl_remap_info_t			index[DB_CFG_MAX_DEV];                      
	struct db_spl_realtime_data_node_t	node;

    struct db_spl_realtime_data_node_t	node_space[DB_CFG_MAX_DATA_NODE_NUM * 3 - 1];                     // 当前RAM 留了1MB， 测点设计师8192. 现在留 8192 * 6 空间作为将来测点扩展. 20260321 修改为 8192 * 3 给采集点

    // 拆分1个region区域给控制点，共 8192 * 1 槽位内存用于db_spl_ctrl_region_t
	union  db_spl_ctrl_node_region_t         ctrl_node_region[1];

	// 剩余2个中的其中1个区域给预留
	struct db_spl_realtime_data_node_t  pad[DB_CFG_MAX_DATA_NODE_NUM];
    // 剩余2个中的其中1个区域给控制队列扩展。  原来设计的 ctrl队列太短了。 因此需要扩展。
    union db_spl_channel_ctr_extend_region_t ctrl_extend_region;
    // 以下为临时结构体，未固定
    struct db_spl_channel_ctr_t         ctrl[DB_CFG_MAX_PORT][DB_CFG_MAX_CTRL_QUEUE];

	struct db_spl_channel_trans_t       trans[DB_CFG_MAX_PORT];

	/* 表尾调试域（与 tbl_t 布局字段一致；绝对偏移随 index[] 长度变化） */
	struct db_spl_debug_region_t        debug;
};




#endif
