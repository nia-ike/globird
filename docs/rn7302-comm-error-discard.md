# RN7302 通信错误丢弃本拍电参/电能方案

## 1. 背景

计量芯片经 SPI 读寄存器时，驱动可能返回 SPI 通信失败、CRC/校验失败等错误（`errno` 如 `ECOMM`、`EBADMSG`）。若应用层忽略错误继续计算：

- **半波**：两拍合成一周波，单拍坏数据会导致 U/I/P 尖刺。
- **全波 / 基波**：电参算错的同时，若仍读取**读后清零**型电能寄存器，本次电能增量会被读走并丢弃，造成电量缺口。

早期半波路径曾用「相对上一拍电压变化超过 20%」做诊断打印（`[HW-JUMP]`），该判据：

- 不能区分通信错误与真实电网突变；
- 易误杀；
- 与驱动已能上报的通信错误重叠。

因此：**取消 20% 软件跳变判据**；统一以驱动读接口失败为准。

## 2. 目标

| 路径 | 驱动读失败时行为 |
|------|------------------|
| 半波 | 本拍不计算；同周期另一半波一并丢弃；`halfwave_seq` 按 start/done/abort 规则，失败不刷新 `halfwave_utc_seq` |
| 全波 | 本拍不计算电参；**不调用** `get_EmuFullEnergy` /（ICHSEL 开时）`get_EmuFundEnergy` |
| 基波 | 本拍不计算电参；**不调用** `get_EmuFundEnergy` |
| 全量 seqlock | 本秒测量失败时 `full_cycle_abort`，不 `done`（不刷新 `update_utc_seq`） |

失败判定：**仅** `MeterChipReadReg` / `MeterChipReadArea` 返回错误（含 SPI、CRC/CS 等驱动 errno），不做幅值百分比比较。

## 3. 方案

### 3.1 半波（成对丢弃 + TIMER 层 seqlock）

```
get_EmuHalf()
  ├─ MeterChipReadArea/Reg 失败 → return -1（不更新 EmuHalf）
  └─ 成功 → 填充缓存，return 0

halfwave_fetch(time)
  ├─ s_hw_discard_pair → 直接失败（同周期丢弃）
  └─ get_EmuHalf 失败 → 置 discard，return -1

rms_algorithm_halfwave()   // 只负责读+算，不碰 seqlock
  ├─ fetch 失败 → return -1
  └─ phasecheck_on/off 计算 → return 0
```

### 3.2 全波 / 基波读接口

```
get_EmuFull(ppEmu) / get_EmuFund(ppEmu)
  ├─ 逐寄存器 MeterChipReadReg，任一步失败 → return -1，*ppEmu=NULL
  │     （失败前不对外发布本拍缓存；成功后再提交）
  └─ 全部成功 → *ppEmu = &EmuFull/EmuFund，return 0
```

电能接口 `get_Emu*Energy()` 保持原样，**由算法层在电参成功后再调用**。

### 3.3 算法层

```
rms_algorithm_fullwave()
  ├─ ICHSEL_OFF：get_EmuFull 失败 → return -1（不读 FullEnergy）
  ├─ ICHSEL_ON ：get_EmuFund 失败 → return -1（不读 FundEnergy）
  └─ 成功后再 get_Emu*Energy + 计算 RemainEnergy 等

rms_algorithm_fundwave()
  ├─ get_EmuFund 失败 → return -1（不读 FundEnergy）
  └─ 成功后再读电能并计算
```

ICHSEL 开启时全波改走基波寄存器：只读 Fund 一路，避免先 Full 再 Fund 的多余 SPI。

### 3.4 测量线程 TIMER 与 seqlock（半波/全波同层、同括号）

半波与全波的 `start/done/abort` **统一放在** `DB_MSR_EVENT_TYPE_TIMER`，算法函数内不碰序号：

```
/* 半波：两拍合成一次发布（约 20ms +2） */
奇数拍:
  halfwave_start()
  rms_algorithm_halfwave()
    ├─ 失败 → halfwave_abort()
    └─ 成功 → 保持奇数，等偶数拍
偶数拍:
  rms_algorithm_halfwave()
    ├─ 已 start 且成功 → halfwave_done()
    └─ 已 start 且失败 → halfwave_abort()

/* 全波：约 1s 一次发布 */
full_cycle_start()
rms_algorithm_Rn7302()
  ├─ 失败 → full_cycle_abort()
  └─ 成功 → full_cycle_done()
```

语义一致：`start`（写前变奇）→ 读算 → `done`（变偶+utc）/ `abort`（回偶、不刷 utc）。

### 3.5 相关文件

| 文件 | 改动要点 |
|------|----------|
| `docs/rn7302-comm-error-discard.md` | 本文档 |
| `meter/.../Rn7302_read.c/.h` | Full/Fund 返回 int；半波删 20% |
| `algorithm_fullwave.c` / `algorithm_fundwave.c` | 先电参后电能；失败早退 |
| `algorithm_halfwave.c` | fetch+计算；不碰 seqlock |
| `db_measure_info.c/.h` | halfwave/full_cycle abort |
| `msr_process.c` | TIMER 层统一半波/全波 start/done/abort |

## 4. 测试方式

### 4.1 半波（通信失败 + 成对丢弃）

- 制造真实 SPI/CRC 失败（线缆干扰、驱动返回错误等），或临时在 `get_EmuHalf` 入口强制 `return -1` 做单次验证。
- 串口可过滤：`grep -E 'HW-ERR|EMU-ERR'`
- 期望：奇数拍读失败后，同周期偶数拍不再参与计算；`halfwave_seq` 不完成有效发布（abort 时 utc 不前进）；恢复后下一完整周期正常 `start/done`。
- **不应再出现** `[HW-JUMP]` / `[HW-PREV]`（20% 软件判据已取消）。

### 4.2 全波 / 基波

| 步骤 | 操作 | 期望 |
|------|------|------|
| 1 | 正常运行约 1s 刷新 | `update_seq` 偶增，`update_utc_seq` 前进；电能量连续 |
| 2 | 制造 SPI/CRC 失败（或临时让 `get_EmuFull`/`get_EmuFund` 首寄存器读失败） | 算法 return -1；日志有 `[EMU-ERR]`/`[HW-ERR]`；**本拍不读电能寄存器** |
| 3 | 观察电能 | 失败拍无「增量被读走后丢弃」；下一成功拍增量连续 |
| 4 | 观察 seq | 失败拍走 abort：`update_utc_seq` 不前进 |
| 5 | ICHSEL 开/关各一轮 | OFF 走 Full+FullEnergy；ON 走 Fund+FundEnergy，失败均不读电能 |

### 4.3 回归

- 无故障时半波约 20ms、全波约 1s 刷新率与数值与改前一致。
- 相序调整开关切换后功率/电能路径正确。

## 5. 非目标

- 不用软件幅值跳变（20% 等）作为丢弃条件。
- 不在本文范围重构 ICHSEL 开启时 Full/Fund 电能是否重复读的历史问题（若存在，另案处理）；本方案保证「电参失败则本拍不读电能」。
