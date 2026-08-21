# edge_alert 告警记录拉取 — 设计文档

## 文档信息

| 项 | 内容 |
|----|------|
| 版本 | v1.0 |
| 日期 | 2026-07-03 |
| 状态 | 设计阶段（待分步实现） |
| 关联 | `edge_alert` 告警服务（已实现入库）、`modbus_slave`、`SmartGate` 配置工具 |

---

## 1. 背景与目标

### 1.1 背景

`edge_alert` 进程已将告警事件写入 `/var/run/record/edge_alert.db`（内存盘 SQLite，断电丢失）。需要在 **SmartGate 上位机** 中查看历史告警，并支持 **本地导出** CSV/Excel 等格式。

### 1.2 目标

1. 上位机通过现有 **Modbus TCP/RTU** 链路拉取告警记录
2. 设备侧 **只读查库 + 分页返回 JSON**，不做 CSV/文件导出
3. `edge_alert` 与查询功能 **解耦**：写库归 `edge_alert`，读库归 `modbus_slave`
4. 查询条件 **仅时间范围 + 分页**（事件流按时间排序）

### 1.3 非目标

| 项 | 说明 |
|----|------|
| 设备侧 CSV/Excel 生成 | 由上位机完成 |
| 设备侧文件分包下载 | 不采用 0x50 等文件传输 |
| 按通道/设备/规则等设备侧过滤 | 维度信息已在每条记录中，上位机本地筛选 |
| 断电 DB 恢复 | DB 在内存盘，重启即空，不做启动恢复 |
| MQTT/云端上送 | 本期不做 |

---

## 2. 总体架构

```
┌─────────────────────────────────────────────────────────────┐
│  SmartGate 上位机                                            │
│  ┌─────────────────┐    ┌─────────────────────────────────┐ │
│  │ 告警记录 UI      │    │ EdgeAlertQueryClient (0x4F)     │ │
│  │ 时间范围 / 分页  │◄──►│ 拉取 JSON → 本地 CSV/Excel 导出  │ │
│  └─────────────────┘    └─────────────────────────────────┘ │
└───────────────────────────────┬─────────────────────────────┘
                                │ Modbus TCP / RTU
┌───────────────────────────────┴─────────────────────────────┐
│  网关 modbus_slave                                           │
│  ┌──────────────────────────────────────────────────────┐ │
│  │ mb_process_4FH → mb_edge_alert_query.c               │ │
│  │                → mb_edge_alert_db.c (只读 SQLite)     │ │
│  └──────────────────────────────────────────────────────┘ │
└───────────────────────────────┬─────────────────────────────┘
                                │ 只读打开（文件级共享）
┌───────────────────────────────┴─────────────────────────────┐
│  /var/run/record/edge_alert.db                                      │
│  table: alarm_records                                        │
└───────────────────────────────▲─────────────────────────────┘
                                │ 读写
┌───────────────────────────────┴─────────────────────────────┐
│  edge_alert 进程（已实现，本期不改查询逻辑）                  │
│  告警引擎 → 状态机 → edge_alert_storage 写库                  │
└─────────────────────────────────────────────────────────────┘
```

### 2.1 职责划分

| 模块 | 职责 |
|------|------|
| `edge_alert` | 告警评估、触发/恢复、极值更新、**写库** |
| `modbus_slave` | 解析 0x4F 请求、**只读查库**、组 JSON 应答 |
| `SmartGate` | 发 0x4F、展示列表、分页拉全量、**本地导出** |

两进程 **无 API 互调**，仅共享 DB 文件路径。

---

## 3. 数据源

### 3.1 数据库

| 项 | 值 |
|----|-----|
| 路径 | `/var/run/record/edge_alert.db` |
| 表名 | `alarm_records` |
| 容量 | 约 8192 条（ID 循环覆盖，`INSERT OR REPLACE`） |
| 索引 | `idx_trigger_ms`、`idx_alarm_key` |

### 3.2 表结构（与 `edge_alert_storage.c` 一致）

| 列名 | 类型 | 说明 |
|------|------|------|
| id | INTEGER PK | 记录 ID（0~8191 循环） |
| alarm_key | TEXT | 告警键，如 `03010001` |
| chnl_id | INTEGER | 通道号 |
| dev_id | INTEGER | 设备号 |
| rule_id | INTEGER | 规则 ID |
| rule_name | TEXT | 规则名称 |
| level | INTEGER | 告警级别 1~4 |
| trigger_ms | INTEGER | 触发时间（毫秒 UTC 或本地 ms，与写库一致） |
| recover_ms | INTEGER | 恢复时间，0=未恢复 |
| trigger_value | REAL | 触发时值 |
| threshold | REAL | 阈值 |
| trigger_point_id | INTEGER | 触发点位 ID |
| recover_value | REAL | 恢复时值 |
| max_value / min_value | REAL | 持续期间极值 |
| max_value_ms / min_value_ms | INTEGER | 极值发生时间 |
| snapshot_json | TEXT | 上下文快照 JSON 字符串 |

### 3.3 记录状态（不上设备协议，上位机推导）

| 状态 | 条件 |
|------|------|
| 活跃 | `recover_ms == 0` |
| 已恢复 | `recover_ms > 0` |

---

## 4. Modbus 功能码

### 4.1 分配

当前业务功能码最大为 **0x4E**（`_FC_GET_EDGE_SCRIPT_HANDLE`）。告警查询使用其后第一个：

```c
/* mb_protocol.h 新增 */
#define _FC_ALARM_QUERY    0x4F
```

| 码 | 名称 | 说明 |
|----|------|------|
| 0x4F | ALARM_QUERY | 统计 + 分页查询 |
| 0x4B ~ 0x4C | （预留） | 不使用 |
| 0x50+ | （未分配） | 本期不使用 |

### 4.2 Op 子命令

| Op | 值 | 说明 |
|----|-----|------|
| `ALARM_OP_STATS` | 0x01 | 时间范围内记录总数 |
| `ALARM_OP_QUERY_PAGE` | 0x02 | 时间范围内分页记录（JSON） |

---

## 5. 报文格式

### 5.1 约定

- 字节序：多字节字段在报文中为 **大端**（与现有 `SWAP_16BIT` / `SWAP_32BIT` 一致）
- RTU：PDU + CRC16
- TCP：MBAP 头 + PDU（与 0x49/0x4E 相同处理方式）
- 单帧 JSON 上限：约 **1200 字节**（受 `MB_COM_RXBUFF_SIZE` 1280 约束）

### 5.2 时间范围 `ModBusAlarmTimeRange`（12 字节）

```c
#pragma pack(push, 1)
typedef struct {
    uint64_t start_ms;   /* 0 = 不限制起始时间 */
    uint64_t end_ms;     /* 0 = 不限制结束时间 */
} ModBusAlarmTimeRange;
#pragma pack(pop)
```

**过滤字段：** 仅 `trigger_ms`（事件发生时间）。

| start_ms | end_ms | 语义 |
|----------|--------|------|
| 0 | 0 | 全库 |
| >0 | 0 | trigger_ms >= start_ms |
| 0 | >0 | trigger_ms <= end_ms |
| >0 | >0 | start_ms <= trigger_ms <= end_ms |

**排序固定：** `ORDER BY trigger_ms DESC, id DESC`（最新在前）。

### 5.3 请求格式

**公共头（4 字节）：**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 0 | SlaveAddr | 1 |
| 1 | Cmd = 0x4F | 1 |
| 2 | Op | 1 |
| 3 | Reserved = 0 | 1 |

**Op = 0x01 STATS**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 4 | TimeRange.start_ms | 8 |
| 12 | TimeRange.end_ms | 8 |
| **合计 PDU** | | **20** |

**Op = 0x02 QUERY_PAGE**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 4 | TimeRange.start_ms | 8 |
| 12 | TimeRange.end_ms | 8 |
| 20 | offset | 2 |
| 22 | limit | 2 |
| **合计 PDU** | | **24** |

- `offset`：从 0 开始，步进为上次 `limit`
- `limit`：建议默认 20，**最大 50**（设备侧钳制，超限返回 `result=3`）

### 5.4 应答格式

**公共头（6 字节）：**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 0 | SlaveAddr | 1 |
| 1 | Cmd = 0x4F | 1 |
| 2 | Op（回显） | 1 |
| 3 | result | 1 |
| 4 | Reserved | 2 |

**result 错误码：**

| 值 | 含义 |
|----|------|
| 0 | 成功 |
| 1 | DB 不可用（文件不存在或打开失败） |
| 2 | SQL 执行失败 |
| 3 | 参数非法（如 limit > 50） |

**Op = 0x01 STATS 成功载荷：**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 6 | total | 4 |

**Op = 0x02 QUERY_PAGE 成功载荷：**

| 偏移 | 字段 | 长度 |
|------|------|------|
| 6 | total | 4 |
| 10 | offset | 2 |
| 12 | count | 2 |
| 14 | json_len | 2 |
| 16 | json[json_len] | 变长 |

---

## 6. JSON 载荷规范

设备 **只做字段序列化**，不做时间格式化、不做 CSV、不解析 `snapshot_json`。

```json
{
  "records": [
    {
      "id": 12,
      "alarm_key": "03010001",
      "rule_name": "电压越上限",
      "level": 1,
      "chnl_id": 3,
      "dev_id": 1,
      "rule_id": 1,
      "trigger_ms": 1751443200123,
      "recover_ms": 0,
      "trigger_value": 236.15,
      "threshold": 220.0,
      "recover_value": 0,
      "max_value": 236.20,
      "min_value": 235.95,
      "max_value_ms": 1751443205000,
      "min_value_ms": 1751443201000,
      "snapshot_json": "{\"A相电流\":12.5}"
    }
  ]
}
```

| 字段 | 说明 |
|------|------|
| trigger_ms / recover_ms | 毫秒时间戳，上位机格式化显示 |
| recover_ms = 0 | 上位机显示「活跃」 |
| snapshot_json | 原样透传 DB 字符串 |
| chnl_id / dev_id / rule_name | 展示与导出列，**不作为查询条件** |

---

## 7. modbus_slave 查库设计

### 7.1 模块与文件

```
modbus_slave/
├── mb_protocol.h              /* 增加 _FC_ALARM_QUERY 0x4F */
├── mb_protocol_tcp.c          /* switch case 增加 mb_process_4FH */
├── mb_protocol_rtu.c          /* 同上 */
├── mb_edge_alert_db.h/.c      /* 只读 DB 连接、SQL 执行 */
└── mb_edge_alert_query.h/.c   /* 4F 报文解析、JSON 组包 */
```

### 7.2 DB 访问

```c
#define MB_EDGE_ALERT_DB_PATH    "/var/run/record/edge_alert.db"

/* 每次请求：打开 → 查询 → 关闭 */
sqlite3_open_v2(MB_EDGE_ALERT_DB_PATH, &db, SQLITE_OPEN_READONLY, NULL);
```

**可选（edge_alert 侧一行配置，非必须）：** 写库时 `PRAGMA journal_mode=WAL;`，降低读写锁冲突。

### 7.3 SQL

#### 时间条件（共用）

```sql
-- start_ms > 0
AND trigger_ms >= :start_ms

-- end_ms > 0
AND trigger_ms <= :end_ms
```

#### Op = 0x01 STATS

```sql
SELECT COUNT(*) FROM alarm_records WHERE 1=1
  [AND trigger_ms >= ?]
  [AND trigger_ms <= ?];
```

#### Op = 0x02 QUERY_PAGE

```sql
-- 1) total（条件同 STATS）
SELECT COUNT(*) FROM alarm_records WHERE 1=1 ...;

-- 2) page
SELECT id, alarm_key, chnl_id, dev_id, rule_id, rule_name, level,
       trigger_ms, recover_ms, trigger_value, threshold,
       recover_value, max_value, max_value_ms, min_value, min_value_ms,
       snapshot_json
FROM alarm_records
WHERE 1=1 ...
ORDER BY trigger_ms DESC, id DESC
LIMIT :limit OFFSET :offset;
```

使用 **预编译语句** 两套模板即可，无需动态拼接维度。

### 7.4 JSON 组包

- 推荐使用 **cJSON**（modbus_slave 工程已具备）
- 逐行 `SELECT` → 追加到 `records` 数组
- 若预估超长，减少本页 `limit` 或截断 `snapshot_json`（可选：超过 N 字符省略尾部并标记）

---

## 8. 上位机（SmartGate）设计

### 8.1 新增组件

| 组件 | 说明 |
|------|------|
| `EdgeAlertQueryClient.cs` | 0x4F 组包/解包，参考 `EdgeScriptLogClient.cs`（49H） |
| `FormEdgeAlertRecords.cs` | 告警记录列表页（时间范围、分页、刷新） |
| 导出入口 | 菜单或按钮「导出 CSV」 |

### 8.2 交互流程

**列表查询：**

```
1. 用户选择时间范围（默认如最近 24 小时）
2. 4F Op=01 → 显示「共 N 条」
3. 4F Op=02 offset=0, limit=20 → 绑定表格
4. 翻页：offset += limit，重复 Op=02
```

**导出（全部在上位机）：**

```
1. 4F Op=01 得 total
2. 循环 Op=02 直至拉取全部 records
3. 合并为 List<AlarmRecordDto>
4. SaveFileDialog → 写 CSV（UTF-8 BOM）或 Excel
5. 列映射、时间格式化、snapshot 展平 — 均由 C# 完成
```

**本地筛选（可选，不占协议）：**

- 表格列过滤器按 `chnl_id`、`rule_name` 等
- 导出前可按当前筛选结果导出

### 8.3 全库导出估算

| 项 | 值 |
|----|-----|
| 最大记录数 | ~8192 |
| 每页 limit | 50 |
| 请求次数 | ~164 次 |
| 建议 | 进度条 + 可取消 |

---

## 9. 边界与异常

| 场景 | 行为 |
|------|------|
| DB 文件不存在 | result=1，上位机提示「无告警数据」 |
| edge_alert 未运行 | 可查已有记录；无新写入 |
| edge_alert 运行中查询 | 只读快照，允许略旧数据 |
| ID 循环覆盖 | 查询结果为当前库内容，属预期 |
| limit 过大 | 设备钳制为 50 或返回 result=3 |
| JSON 超帧长 | 减小 limit 重试 |
| 49H 日志调试 | 独立功能码，互不影响 |

---

## 10. 实施计划（分步）

### Phase 1 — modbus_slave 设备侧

- [ ] `mb_protocol.h` 增加 `_FC_ALARM_QUERY 0x4F`
- [ ] `mb_edge_alert_db.c` 只读连接 + STATS / PAGE SQL
- [ ] `mb_edge_alert_query.c` Op=01/02 + cJSON 组包
- [ ] `mb_protocol_tcp.c` / `mb_protocol_rtu.c` 注册 `mb_process_4FH`
- [ ] 本地用模拟数据或真实 `edge_alert.db` 联调

### Phase 2 — SmartGate 上位机

- [ ] `EdgeAlertQueryClient.cs`
- [ ] 告警记录 UI（时间、分页、表格列）
- [ ] 本地 CSV 导出

### Phase 3 — 联调与 polish

- [ ] 与运行中 `edge_alert` 端到端测试
- [ ] 大数量分页拉取与导出性能
- [ ] 中英文 UI 文案

### 不涉及（本期）

- [ ] `edge_alert` 进程代码变更（查询逻辑）
- [ ] 设备侧 CSV / 0x50 文件下载
- [ ] 多维度设备侧 Filter

---

## 11. 测试要点

| 编号 | 用例 |
|------|------|
| T1 | 空库：STATS total=0，PAGE 空 records |
| T2 | 仅 start_ms / 仅 end_ms / 闭区间 |
| T3 | 分页：total=100，limit=20，offset 0/20/80 |
| T4 | limit=51 → result=3 |
| T5 | 触发+恢复各一条，JSON 字段完整 |
| T6 | edge_alert 写入同时 modbus_slave 查询不崩溃 |
| T7 | 上位机拉全库 8192 条并导出 CSV |

---

## 12. 修订记录

| 版本 | 日期 | 说明 |
|------|------|------|
| v1.0 | 2026-07-03 | 初版：解耦架构、0x4F 仅时间查询、上位机导出 |

---

## 附录 A：与旧方案差异摘要

| 项 | 早期草案 | 本文档定稿 |
|----|----------|------------|
| 读库位置 | 曾考虑 edge_alert 提供 API | **仅 modbus_slave 只读 DB** |
| 功能码 | 曾用 0x4B/0x4C | **0x4F**（0x4B/0x4C 预留） |
| 设备 CSV | 曾考虑 PREPARE + 0x50 下载 | **取消** |
| 查询维度 | 通道/设备/规则/级别/状态 | **仅时间 + 分页** |
| 导出 | 设备生成 CSV | **上位机拉 JSON 后本地导出** |
