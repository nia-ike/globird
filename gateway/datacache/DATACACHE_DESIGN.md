# DataCache 新设计说明（上位机 / 需求对接）

> 适用模块：设备端 `data_cache`（`app/datacache`）  
> 配置路径：`/userdata/config/project/data_cache.json`  
> 文档用途：指导上位机配置下发、模式选择、文件拉取与断点续传策略调整  

---

## 1. 背景与目标

DataCache 将实时库快照（LZ4 压缩）写入 SQLite，供：

1. **MQTT 等协议断点续传**（短时环形缓存，偏 RAM）
2. **按日历史归档**（落 NAND，供上位机按天回读）

二者需求冲突（跨天连续 vs 自然日切分、NAND 寿命），因此采用 **模式二选一**，不做双份 RAM 镜像。产品定位非专用 datalogger，允许有限能力。

---

## 2. 两种模式（互斥）

| 模式 | 配置值 | 用途 | 存储行为 |
|------|--------|------|----------|
| **滚动续传** | `mode`: `"roll"` / `"resume"` / `0` | MQTT 等断点续传 | RAM 连续追加；超高水位删旧到低水位；**可跨天**（容量内） |
| **按日归档** | `mode`: `"daily"` / `"day"` / `"today"` / `1` | 按天历史、上位机按日下载 | 只攒当天；达高水位**停止写入**；零点归档到 NAND 后清空当日库 |

### 2.1 滚动模式（roll）

- 文件：`/var/run/cache/data.db`（一般在 tmpfs，重启可能清空）
- 写入：持续追加压缩快照
- 容量：超过 **高水位** → 删除最旧记录并 `VACUUM`，落到 **低水位**
- 续传：可在容量窗口内跨天；窗口长度取决于测点数、间隔、压缩率
- **不保证**按自然日完整；**默认不写**按日历史目录（历史能力以日模式为准）

### 2.2 按日模式（daily）

- 当日仍写：`/var/run/cache/data.db`
- 达到高水位后：**当天不再写入**（“一天只存这么多”）
- 日期变化（本地日历日变更）时：
  1. 将当前库拷贝为：`{history_path}/YYYYMMDD.db`（归档**昨天**）
  2. **立刻**重建空的当日 `data.db`（**跨天未完成的断点续传视为放弃**）
  3. 再 gzip 为 `{history_path}/YYYYMMDD.db.gz` 并删除明文；gzip 失败则保留明文以免丢档
  4. 删除超出 `history_days` 的最旧日期文件
- 上位机按日回读：读 NAND 上已归档的 `.db.gz`，下载后解压；运行时 `data.db` 始终明文

---

## 3. 路径与文件

| 路径 | 说明 |
|------|------|
| `/userdata/config/project/data_cache.json` | 运行配置 |
| `/var/run/cache/data.db` | 当前活动缓存（两模式共用此路径） |
| `/userdata/history/YYYYMMDD.db.gz` | 日模式归档（默认目录，可用配置改） |

归档文件名规则：`YYYYMMDD.db.gz`（例：`20260817.db.gz`）。列表与下载协议只公告/传输 gzip，不兼容明文 `.db`。

表结构（逻辑）：

| 列 | 含义 |
|----|------|
| `_Id` | 单调递增序号（续传游标可用） |
| `_Time` | 写入时间（本地） |
| `_Range` | 与 `_Id` 相同（兼容旧表主键语义） |
| `_Bin` | LZ4 压缩的实时库快照 BLOB |

解码方式与既有 datacache / Modbus 拉库一致：解压 `_Bin` 后按实时库节点布局解析。

---

## 4. 配置文件

### 4.1 新配置完整示例（推荐）

```json
{
  "attr": {
    "checked": 1,
    "inv_time": 300,
    "max_items": 8192,
    "max_rows": 2016,
    "max_alarm_nums": 256,
    "mode": "roll",
    "high_mb": 10,
    "low_mb": 9,
    "history_days": 8,
    "history_path": "/userdata/history"
  }
}
```

### 4.2 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|------|------|
| `checked` | int | 旧有 | `1` 使能缓存；`0` 进程空转不写 |
| `inv_time` | int | 旧有 | 采样间隔（秒）。`≥60` 按分钟对齐；`<60` 按秒对齐。`0` 会被改成 `60` |
| `max_items` / `max_rows` / `max_alarm_nums` | int | 旧有 | **已不参与容量计算**，保留兼容 |
| `mode` | string/int | 新 | 见第 2 节；缺省 = `roll` |
| `high_mb` | int | 新 | 高水位（MB） |
| `low_mb` | int | 新 | 低水位（MB），**仅 roll 裁剪有效**；若只配 `high_mb`，默认 `high_mb - 1` |
| `history_days` | int | 新 | 日模式 NAND 保留天数，默认 `8` |
| `history_path` | string | 新 | 日模式归档目录，默认 `/userdata/history` |

### 4.3 旧配置兼容（现场升级）

旧 JSON 示例（无新字段）：

```json
{"attr":{"checked":1,"inv_time":300,"max_items":8192,"max_rows":2016,"max_alarm_nums":256}}
```

设备端行为：

| 参数 | 缺省（旧设计兼容） |
|------|--------------------|
| `mode` | `roll` |
| 高水位 | **8 MB** |
| 低水位 | **6 MB** |
| `history_days` | 8 |
| `history_path` | `/userdata/history` |

> **注意**：未改设备上 JSON 时，运行日志会显示 `high_size: 8 MB / low_size: 6 MB`，这是兼容默认，不是新推荐值 10/9。  
> 要用 10MB/9MB，必须在设备配置中显式写上 `high_mb` / `low_mb`。

### 4.4 上位机配置界面建议

1. **模式**：单选「滚动续传」/「按日归档」，互斥，切换后提示重启 `data_cache` 或清库。
2. **水位**：滚动模式显示高/低水位（MB）；按日模式可只显示「当日上限（高水位）」，低水位可灰显。
3. **按日参数**：仅日模式可编辑「保留天数」「历史目录」。
4. **下发**：写入 `/userdata/config/project/data_cache.json` 后重启缓存进程。
5. **兼容**：读到旧文件时，UI 按 8/6 + roll 显示，并提示“升级配置可改为 10/9”。

---

## 5. 水位与容量（给估算用）

| 场景 | 高水位 | 低水位 | 行为 |
|------|--------|--------|------|
| 新配置推荐 | 10 MB | 9 MB | roll：超 10 裁到约 9；daily：满 10 停写 |
| 旧 JSON 兼容 | 8 MB | 6 MB | 同上逻辑，容量更小 |

说明：

- 已取消按测点数分档固定条数（原 8192/4096/…）；条数由压缩后体积与水位决定。
- 测点少、压缩高时，同样水位可存**更多条**（相对旧分档设计往往多几倍）。
- 间隔多为分钟级时，单日数据通常远小于高水位；日模式“停写”多为异常保护。

---

## 6. 上位机功能改动清单（需求）

### 6.1 必须

| # | 需求 | 说明 |
|---|------|------|
| R1 | 支持下发/读取新配置字段 | `mode`、`high_mb`、`low_mb`、`history_days`、`history_path` |
| R2 | 模式互斥与文案 | 明确 roll=续传优先；daily=按日归档优先，跨天续传不保留 |
| R3 | 旧配置兼容展示 | 无新字段时按 8MB/6MB + roll 解释，勿显示成 10/9 |
| R4 | 日模式历史拉取 | 列出 `{history_path}` 下 `YYYYMMDD.db.gz`，按日下载 gzip 后解压再分析（Modbus 46H destinype=3） |
| R5 | 续传策略随模式切换 | roll：沿用现有对 `/var/run/cache/data.db` 的续传；daily：仅当日有效，换日需重置游标 |

### 6.2 建议

| # | 需求 | 说明 |
|---|------|------|
| S1 | 切换模式时引导清缓存/重启 | 避免旧游标与文件语义混乱 |
| S2 | 日模式显示“当日已满” | 设备达高水位停写后，上位机可提示当天不再追加 |
| S3 | 历史保留天数与磁盘告警 | 与 `history_days` 一致；目录异常时提示 |
| S4 | 调试 | 设备可用 `data_cache -d` 打印 mode / high_size / low_size / db 大小 |

### 6.3 不做 / 有限能力（产品约定）

- 不在 roll 模式下保证「自然日完整历史」。
- 不在 daily 模式下保证「跨天断点续传」。
- 不做 RAM 双库（`data.db` + `today.db`）以节省 128MB 级内存产品。
- 非专用 datalogger：不承诺秒级海量点的完整天数据。

---

## 7. 通讯与数据流（示意）

```
┌─────────────┐     快照+LZ4      ┌──────────────────────┐
│ 实时库 vmem │ ───────────────► │ /var/run/cache/data.db│
└─────────────┘                   └──────────┬───────────┘
                                             │
                    mode=roll                │ mode=daily
                    水位滚动                 │ 满则停写
                    MQTT 续传 ◄──────────────┤
                                             │ 零点切日
                                             ▼
                                  ┌──────────────────────┐
                                  │ /userdata/history/   │
                                  │   YYYYMMDD.db.gz × N │
                                  └──────────┬───────────┘
                                             │
                                             ▼
                                        上位机按日下载
```

现有拉「当前缓存库」的通道（Modbus 46H destinype=2）对运行时 `data.db` 先 `cp` 再 `gzip -1` 后传输；上位机解压后再当 SQLite 打开。  
**daily** 下：当天未归档数据在 RAM `data.db`；已过去的天在 `history_path` 的 `.db.gz`。

---

## 8. 验收要点

1. **旧 JSON**：升级程序后不改配置 → `mode=roll`，水位 8/6，续传可用。  
2. **新 JSON roll**：`high_mb=10, low_mb=9` → 日志为 10/9；库体积在水位间滚动。  
3. **新 JSON daily**：写满高水位后不再增加；跨日后出现 `history/YYYYMMDD.db.gz`，当日库清空；超过 `history_days` 的旧文件被删。  
4. **上位机**：可配置模式与水位；可按日列出/下载历史文件；模式切换后续传游标行为符合第 6 节。

---

## 9. 相关代码位置（设备端）

| 内容 | 路径 |
|------|------|
| 进程入口 / 切日 | `app/datacache/main.c` |
| 写入 / 水位 / 归档 | `app/datacache/dc_sql_cache.c` |
| 配置加载与兼容 | `app/datacache/config/dc_cfg_attr.c` |
| 示例配置 | `app/datacache/data_cache.json` |
| 既有缓存文件下载参考 | `app/modbus_slave/mb_datacache_file.*`（源 `/var/run/cache/data.db`，传输 `.db.gz`） |

---

## 10. 修订记录

| 日期 | 说明 |
|------|------|
| 2026-08-17 | 初版：取消分档条数；容量水位滚动；roll/daily 二选一；配置扩展与旧 JSON 兼容；日模式 NAND 归档与上位机需求 |
