/*
 * File      : modbus-cfg.c
 * This file is part of xgate5-fire
 * COPYRIGHT (C) 2006 - 2018, 19 development, Communication team, chenzy.
 *
 * 
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-09     chenzy       channel-device-node setting.
 */

#ifndef __DLT645_97_CFG_H__
#define __DLT645_97_CFG_H__

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <unistd.h> 
#include <sys/stat.h>  //stat
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <errno.h>

#include "cJSON.h"
#include "cJSON_Utils.h"





#define CONFIG_NODE_NAME_LEN				12			/* 测点名称长度 */
#define CONFIG_MAX_REQUEST_PKT_LEN			16

#define CFG_MAX_POINT					    256			/* 单台智能设备最大测点数 */
#define CONFIG_MAX_CTRLNODESIZE_PER_DEVICE	8           /* 单台智能设备最大控制点数 */
#define CONFIG_MAX_ALARMRULE_PER_DEVICE     32          /* 单台智能设备最大告警规则数 */


struct  tagDataM6
{
  uint8_t n;			    /* 数据块*/
  uint8_t tm_min;			/* Minutes.	[0-59] */
  uint8_t tm_hour;			/* Hours.	[0-23] */
  uint8_t tm_mday;			/* Day.		[1-31] */
  uint8_t tm_mon;			/* Month.	[0-11] */
  uint8_t tm_year;			/* Year	- 1900.  */
};
/* 
	读报文请求
*/
struct config_pkt_request_info
{
	uint8_t 						code;
	uint32_t						reg_no;
	uint16_t						reg_num;
};

struct config_pkg_request
{
	uint16_t							pkt_num;
	struct config_pkt_request_info		pkt_req_info[CONFIG_MAX_REQUEST_PKT_LEN];
};


/*
	测点属性
*/
struct config_node_attr
{
	uint16_t				id;					/* id */
	uint8_t				datatype;			/* 数据类型 */
	uint8_t				leadnum;			/* 前导符个数*/
	uint8_t				code;				/* 功能码 */
	uint8_t				datalen;			/* 数据长度 */
	struct  tagDataM6	data;				/* 数据 */
//	uint8_t				bitfield;			/* 位域 */
//	uint8_t				flag;				/* bit[2:0] AI-DI-ACC   */
	uint32_t			reg_no;				/* 寄存器号 */
	uint32_t			catagory;			/* 测点类别，如A相电压 */
	float				kfactor;			/* k 系数 */
	float				pt_flag1;			/* PT  0XFFFF 为需要修改的值， 非0XFFFF 为固定值 */
	float				ct_flag2;			/* CT  0XFFFF 为需要修改的值， 非0XFFFF 为固定值 */
//   char 				name[12];			/* 测点名称 */
//   char				unit[8];			/* 单位 */

		// add by chenzy 20240630
	uint8_t			    range_checked;		/* 数据范围清洗使能  0-关闭   1-满足范围该点值为1，否则为0   2-满足该范围才更新值 */
	int                 range_min;
	int                 range_max;

	uint16_t            dataoffset;			/* 从数据域其实的第dataoffset个开始解析本测点 */
};


/*------------------     控制结构  ------------------ */
struct config_control_attr
{
	uint8_t                         id;  /*id*/
	uint8_t                         datatype; /*数据类型*/
	uint8_t							funcode;  /*功能码*/
	uint32_t						regno;    /*寄存器地址*/
	uint16_t						leadbytenum;/*前导符个数*/
	uint32_t						password;  /*密码*/
	uint32_t						operator;	/*操作员编码*/	
	uint16_t 						regvalue;  /*写入的值*/
	/*
		更加控制形式
		1.有些设备的遥控使用的是14H功能码进行 合闸分闸操控。因此需要设定对应写命令和写寄存器	
		2.本设备的遥控遥调 支持对应的 01 03 功能码，自动转换成 南向的05H  06H 10H功能码，但局限性就是需要按标准modbus的设备才能用：
		（比如继电器能用01读， 05写）但有的设备不支持01读，或者读写不在同一个寄存器

		以上情况，需要用下面的设定寄存器来实现
	*/
	uint32_t 						regno0;  //
	uint32_t 						regno1;  //
	uint16_t 						regvalue0;  //
	uint16_t 						regvalue1;  //

};


/*------------------     告警设置结构  ------------------ */
struct alarm_condition
{
    uint32_t        pointid;                    //本规则使用的本测点ID
    char            type[12];                   //规则类别，固定为"const"
    uint32_t        level;                      //告警等级，1 一般 2 严重 3紧急
    uint8_t         method;                     //告警方式，upper上限(0x1)、up upper上上限(0x2)或者lower下限(0x4)、low lower下下限(0x8)
    double          ref;                        //额定参考值n，用作百分比越限时的额定参考。
    uint8_t         ref_per;                    //越限范围，额定参考值的百分比(120 -> 120%)
    uint8_t         Hysteresis;                 //回滞值n。单位百分比。上限时，为min(1-n), 下限时，为min(1+n)
};

typedef struct config_alarm_attr
{
    uint32_t        ruleid;     //告警规则ID,关联实时告警上传值
    //char            name[32];   //告警名称
    char            type[16];   //告警类别，如漏电告警
    uint8_t         mode;       //组合模式: 0: single-单点告警 1: dual-双点组合告警
    uint32_t        htime;      //告警判断保持时间（越限持续时间达到设定值才告警），单位 秒
    struct alarm_condition alarm_cond[2];   //告警条件，mode为single时，单点上限、上上限，下限、下下限；mode为dual时，双点两条件
}config_alarm_attr, *config_alarm_attr_t;


/* 设备点表属性 */
struct config_devices_attr
{
	uint8_t							checked;												/* 本设备有效 */
//    char                            sn[16];
//    char                            model[16];
//    char                            name[64];
//    char                            function[16];
    
	struct config_pkg_request		pkg_request;											/* 本设备读请求报文 */
		
	uint16_t						node_num;												/* 本设备节点数 ，即node_attr 有效数量         */
	struct config_node_attr			node_attr[CFG_MAX_POINT];
	uint8_t                         control_num;                                                /* 电表控制点个数 */
	struct config_control_attr 		control_attr[CONFIG_MAX_CTRLNODESIZE_PER_DEVICE];			/* 电表控制点设置 */
    uint8_t                        	alarm_num;                                              /* 本设备告警规则条数 */
    struct config_alarm_attr        alarm_attr[CONFIG_MAX_ALARMRULE_PER_DEVICE];            /* 本设备告警规则 */
};


int dlt645_97_cfg_load(struct config_devices_attr *pCfg, char *file);
int32_t dlt645_97_cfg_node_attr_size(struct config_devices_attr *pDevAttr);
int32_t dlt645_97_cfg_nodectrl_attr_size(struct config_devices_attr *pDevAttr);

struct config_node_attr *dlt645_97_cfg_node_attr(struct config_devices_attr *pDevAttr, uint8_t id);
struct config_control_attr *dlt645_97_cfg_nodectrl_attr(struct config_devices_attr *pDevAttr, uint8_t id);

#endif

