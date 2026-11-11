# CT 反向与 ICHSEL 相序调整

## 1. 背景

`PhaseAdjust1/2/3` 同时编码两件事：

| 位域 | 含义 |
|------|------|
| 低 3 位 `1/2/3` | 物理电流通道 A/B/C 映射 |
| bit15 `0x8000` | CT 极性反 |

原先 `Rn7302_ICHSEL_Init()` 用整值比较 `1 && 2 && 3` 才关硬件换相。仅 CT 反向时配置为 `0x8001 / 2 / 3` 等，也会被当成「相序已调整」而打开 `ICHSEL_ON`。

ICHSEL 开启后有两类精度偏差：

1. **半波**：改走电压、电流有效值与角度计算功率（`phasecheck_on`），而不是读芯片半波功率寄存器。
2. **全波 / 基波**：全波改读计量芯片基波寄存器来顶全波数据；脉冲也从全波 P/Q 切到基波 FP/FQ。

CT 极性本身不靠 ICHSEL 硬件处理，软件已有两条路径：

- `rms_algorithm_currentangle()`：bit15 时电流角 +180°。
- `rms_algorithm_CurrentReverse()`：P3W4 下按 bit15 翻转 P/Q/PF 符号。

全波/基波在 `rms_algorithm_Rn7302()` 里算完后会调 `CurrentReverse`；半波 TIMER 路径原先没有，CT 反向时半波功率符号错误。

## 2. 目标

| 场景 | ICHSEL | 半波功率 | 电流角 | CurrentReverse |
|------|--------|----------|--------|----------------|
| 相序 ABC，可带 CT 反向 | OFF | 芯片半波功率寄存器 | `currentangle`，bit15 则 +180° | 半波一次值在转二次前调用 |
| 真正换相（可带 CT 反向） | ON | V/I/角计算 | 换通道 + 可选 +180° | **不调**（角已含极性，再翻会双反） |

判定「是否真正换相」只看低 3 位是否为 `1/2/3`，忽略 `0x8000`。

## 3. 方案

### 3.1 ICHSEL 初始化：低 3 位为 1/2/3 则不开硬件换相

```
Rn7302_ICHSEL_Init()
  phase[i] = PhaseAdjust(i+1)
  if ((phase[0]&0x07)==1 && (phase[1]&0x07)==2 && (phase[2]&0x07)==3)
      ICHSEL_OFF：关 TFEEC / ICHSEL，走芯片全波/半波功率
  else
      ICHSEL_ON ：开通道切换，ICHSEL 写入仍用 phase[i]&0x3F（自然去掉 0x8000）
```

仅 CT 反向（如 `0x8001,2,3`）与默认 `1,2,3` 一样保持 `ICHSEL_OFF`，避免半波用 V/I/角、全波用基波寄存器。

### 3.2 半波 ICHSEL_OFF：转二次前翻功率符号

```
rms_algorithm_halfwave_phasecheck_off()
  电压/电流/角
  芯片半波 P/Q → S → PF
  CurrentReverse(out)          // 翻一次值 P/Q/PF；不改 J_Ia
  Primary_to_secondary         // 二次值跟一次值符号
```

电流角仍由 `rms_algorithm_currentangle()` 处理，`CurrentReverse` 不改相角。

`phasecheck_on` 功率已用含 +180° 的 `J_Ia` 计算，**不再**调 `CurrentReverse`。

调用必须在功因之后、一次转二次之前：与全波顺序一致；若先翻 P 再算 PF，再被 `CurrentReverse` 翻一次 PF，功因会反两次。

### 3.3 相关文件

| 文件 | 改动要点 |
|------|----------|
| `docs/rn7302-ct-reverse-ichsel.md` | 本文档 |
| `meter/.../Rn7302_phasecheck.c` | 低 3 位 1/2/3 即 ICHSEL_OFF |
| `meter/.../algorithm_halfwave.c` | OFF 路径功因后、转二次前调用 CurrentReverse |

## 4. 测试方式

### 4.1 仅 CT 反向（相序仍为 ABC）

| 步骤 | 操作 | 期望 |
|------|------|------|
| 1 | `PhaseAdjust` = `0x8001, 2, 3`（或 B/C 单相反、三相都反） | `get_ICHSEL_flag() == ICHSEL_OFF`；TFEEC/ICHSEL 为默认关闭 |
| 2 | 半波 | 功率走芯片半波寄存器；反向相 P/Q/PF 符号与全波一致；`J_Ix` +180° |
| 3 | 全波 / 基波 | 读全波寄存器，不改走基波顶全波；脉冲仍为全波 P/Q |
| 4 | 二次值 | 半波 secondary 与 primary 符号一致 |

### 4.2 真正换相

| 步骤 | 操作 | 期望 |
|------|------|------|
| 1 | `PhaseAdjust` = `2,3,1` 或 `0x8002,3,1` | `ICHSEL_ON`；半波走 V/I/角功率 |
| 2 | 带 `0x8000` 的换相 | 半波 **不**再调 CurrentReverse；功率符号只来自角 +180°，与全波（芯片功率 + CurrentReverse）一致 |

### 4.3 回归

- `1,2,3` 无极性：半波/全波路径与改前一致，不调用有效翻转。
- 无故障时半波约 20ms、全波约 1s 刷新率不变。

## 5. 非目标

- 不改 `CurrentReverse` 仅处理 P3W4 的既有范围。
- 不把电流角翻转入 `CurrentReverse`（角已在 `currentangle`）。
- 不在本文范围调整 ICHSEL_ON 时全波改读基波的历史策略；仅保证「未换相（含仅 CT 反向）」不再误开 ICHSEL。
