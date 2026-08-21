/**
 * @file anti_reflux_model.h
 * @brief 防逆流策略设备模型：电表与逆变器测点及 dblink_id
 *
 * 依据 strategy_control.json 的 MeterRef/InverterRefs，从 channels 与设备 cfg_file
 * 的 strategy_points 加载测点，生成 dblink_id 供策略调控服务读写。
 *
 * dblink_id 编码：ChnlId(高8bit) | DevIndex(8bit) | datattr(8bit) | slot_index(低8bit)；
 * slot_index 为 -1 时表示测点未绑定，dblink_id = ANTI_REFLUX_DBLINK_ID_INVALID。
 */

#ifndef ANTI_REFLUX_MODEL_H
#define ANTI_REFLUX_MODEL_H

#include <stdint.h>

/*-----------------------------------------------------------------------------
 * dblink_id 编码与常量
 *-----------------------------------------------------------------------------*/

/** 测点未绑定或无效时的 dblink_id */
#define ANTI_REFLUX_DBLINK_ID_INVALID    (0xFFFFFFFFU)

/**
 * 根据 ChnlId、DevIndex、datattr、slot_index 生成 dblink_id。
 * slot_index < 0 时返回 ANTI_REFLUX_DBLINK_ID_INVALID。
 */
#define ANTI_REFLUX_DBLINK_ID_BUILD(chnl_id, dev_index, datattr, slot_index)  \
    (((int)(slot_index)) < 0 ? ANTI_REFLUX_DBLINK_ID_INVALID :                \
     (uint32_t)(((uint32_t)((chnl_id)   & 0xFFU) << 24) |                    \
                ((uint32_t)((dev_index) & 0xFFU) << 16) |                    \
                ((uint32_t)((datattr)   & 0xFFU) <<  8) |                    \
                ((uint32_t)((slot_index) & 0xFFU))))

/** 从 dblink_id 解析 ChnlId（高 8 位） */
#define ANTI_REFLUX_DBLINK_GET_CHNL_ID(dblink_id)     (((uint32_t)(dblink_id) >> 24) & 0xFFU)
/** 从 dblink_id 解析 DevIndex */
#define ANTI_REFLUX_DBLINK_GET_DEV_INDEX(dblink_id)   (((uint32_t)(dblink_id) >> 16) & 0xFFU)
/** 从 dblink_id 解析 datattr */
#define ANTI_REFLUX_DBLINK_GET_DATATTR(dblink_id)     (((uint32_t)(dblink_id) >>  8) & 0xFFU)
/** 从 dblink_id 解析 slot_index（低 8 位） */
#define ANTI_REFLUX_DBLINK_GET_SLOT_INDEX(dblink_id)  ((uint32_t)(dblink_id) & 0xFFU)

/*-----------------------------------------------------------------------------
 * 策略测点项：支持 slot 绑定 与 用户填写 两种配置
 * JSON: value_source="slot" 用 slot_index/dblink_id；value_source="user_fill" 用 user_value_num
 *-----------------------------------------------------------------------------*/

#define ANTI_REFLUX_POINT_NAME_LEN       32

/** 单个策略测点：slot 绑定时用 dblink_id 读写；用户填写时用 user_value_num（仅读点有效） */
typedef struct {
    char        point_name[ANTI_REFLUX_POINT_NAME_LEN];  /**< 测点名称（与 strategy_points 的 key 对应） */
    uint16_t    point_index;     /**< 测点序号（在 read[]/write[] 中的下标） */
    uint16_t    slot_index;      /**< 设备点表 slot 下标，-1 表示未绑定 */
    uint16_t    datattr;         /**< 数据类型属性（如 0=AI, 1=DI, 3=DO, 4=AO） */
    uint32_t    dblink_id;       /**< 运行时 dblink_id，未绑定时为 ANTI_REFLUX_DBLINK_ID_INVALID */
    uint8_t     is_user_fill;    /**< 1=用户填写，取值用 user_value_num；0=slot 绑定 */
    double      user_value_num;  /**< 用户填写时的数值（仅 is_user_fill 时有效，如额定功率 50000 W） */
} AntiRefluxStrategyPoint;

/*-----------------------------------------------------------------------------
 * 电表策略模型（防逆流固定 read 点，无 write）
 * 对应 docs/meter_anti_reflux_model.json strategy_points.group[].read[]
 *-----------------------------------------------------------------------------*/

#define ANTI_REFLUX_METER_READ_POINTS   11

/** 电表防逆流读点序号枚举（与 strategy_points.read[] 顺序一致） */
typedef enum {
    ANTI_REFLUX_METER_READ_Ua = 0,
    ANTI_REFLUX_METER_READ_Ub,
    ANTI_REFLUX_METER_READ_Uc,
    ANTI_REFLUX_METER_READ_Ia,
    ANTI_REFLUX_METER_READ_Ib,
    ANTI_REFLUX_METER_READ_Ic,
    ANTI_REFLUX_METER_READ_Pa,
    ANTI_REFLUX_METER_READ_Pb,
    ANTI_REFLUX_METER_READ_Pc,
    ANTI_REFLUX_METER_READ_Ptot,
    ANTI_REFLUX_METER_READ_Freq,
    ANTI_REFLUX_METER_READ_COUNT
} AntiRefluxMeterReadPointIndex;

/** 电表防逆流策略模型：来自 MeterRef + 设备 cfg strategy_points */
typedef struct {
    int32_t     ChnlId;         /**< 通道号（与 strategy_control.json MeterRef 一致） */
    int32_t     DevIndex;       /**< 设备号（与 chnl_N.json 中 attr.devices.attr[].logic_id 匹配） */
    AntiRefluxStrategyPoint  read_points[ANTI_REFLUX_METER_READ_POINTS];  /**< 11 个只读测点 */
} AntiRefluxMeterModel;

/*-----------------------------------------------------------------------------
 * 逆变器策略模型（防逆流固定 read + write 点）
 * 对应 docs/inverter_anti_reflux_model.json strategy_points.group[].read[] / write[]
 *-----------------------------------------------------------------------------*/

#define ANTI_REFLUX_INV_READ_POINTS     4
#define ANTI_REFLUX_INV_WRITE_POINTS    4

/** 逆变器防逆流读点序号 */
typedef enum {
    ANTI_REFLUX_INV_READ_CommStatus = 0,
    ANTI_REFLUX_INV_READ_ActivePower,
    ANTI_REFLUX_INV_READ_LimitPower,
    ANTI_REFLUX_INV_READ_RatedPowerW,
    ANTI_REFLUX_INV_READ_COUNT
} AntiRefluxInverterReadPointIndex;

/** 逆变器防逆流写点序号 */
typedef enum {
    ANTI_REFLUX_INV_WRITE_PowerLimitEnable = 0,
    ANTI_REFLUX_INV_WRITE_Start,
    ANTI_REFLUX_INV_WRITE_Stop,
    ANTI_REFLUX_INV_WRITE_PowerLimitValue,
    ANTI_REFLUX_INV_WRITE_COUNT
} AntiRefluxInverterWritePointIndex;

/**
 * strategy_points.group[].read_write[] 中「动态防逆流开关」的 key。
 * 绑定虚拟 DO/AO：实时值为剩余抑制秒数 S；S>0 时本周期不参与统筹，S==0 时允许（在 persist_enabled 为真时）。
 * 未绑定有效 slot（或仅为 user_fill）时动态层不生效，仅由 strategy_control.json InverterRefs[].enable 决定。
 */
#define ANTI_REFLUX_RW_KEY_DYNAMIC_CONTROL  "DynamicAntiRefluxEnable"

/** 单台逆变器防逆流策略模型 */
typedef struct {
    int32_t     ChnlId;         /**< 通道号 */
    int32_t     DevIndex;       /**< 设备号（与 chnl_N.json 中 attr.devices.attr[].logic_id 匹配） */
    uint8_t     persist_enabled; /**< 1=参与防逆流（InverterRefs[].enable，缺省 1）；0 时不加载设备 strategy_points，不参与统筹 */
    uint8_t     safety_relay_present; /**< 1=存在 safety_relay_bind 对象（即使 enabled=false） */
    uint8_t     safety_relay_enabled; /**< 1=启用继电器安全防线绑定 */
    int32_t     safety_relay_chnlid;  /**< 继电器通道号 */
    int32_t     safety_relay_devindex;/**< 继电器设备号 */
    uint16_t    safety_trip_bit0_15;  /**< 分闸写点 bit0-15 */
    uint16_t    safety_close_bit0_15; /**< 合闸写点 bit0-15 */
    uint16_t    safety_state_bit0_15; /**< 状态读点 bit0-15 */
    uint32_t    safety_trip_dblink_id; /**< 分闸写点完整 dblink_id */
    uint32_t    safety_close_dblink_id;/**< 合闸写点完整 dblink_id */
    uint32_t    safety_state_dblink_id;/**< 状态读点完整 dblink_id */
    AntiRefluxStrategyPoint  read_points[ANTI_REFLUX_INV_READ_POINTS];   /**< 4 个只读测点 */
    AntiRefluxStrategyPoint  write_points[ANTI_REFLUX_INV_WRITE_POINTS]; /**< 4 个只写测点 */
    /** 动态抑制：read_write[] key=DynamicAntiRefluxEnable；无效 dblink 或未绑定时动态层失效 */
    AntiRefluxStrategyPoint  rw_dynamic_control;
    double      limit_power_step_w; /**< 限功率颗粒度 W，由用户配置；0 表示未配置或连续可调 */
} AntiRefluxInverterModel;

/*-----------------------------------------------------------------------------
 * 防逆流拓扑：1 电表 + n 台逆变器（n 为 1~32）
 *-----------------------------------------------------------------------------*/

#define ANTI_REFLUX_MAX_INVERTERS        32

/** 防逆流策略设备模型：电表 + 多台逆变器，均已加载 strategy_points 并填充 dblink_id */
typedef struct {
    AntiRefluxMeterModel     meter;      /**< 电表策略模型（1 个） */
    AntiRefluxInverterModel  inverters[ANTI_REFLUX_MAX_INVERTERS];  /**< 逆变器策略模型列表 */
    uint32_t                 inverter_count;   /**< 实际逆变器数量，1~32 */
} AntiRefluxDeviceModel;

#endif /* ANTI_REFLUX_MODEL_H */
