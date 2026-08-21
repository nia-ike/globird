# strategy_points：`read_write` 读写绑定（逆变器）

与 `read[]`、`write[]` **同级**，位于 `attr.strategy_points.group[]` 中 `strategy_id=anti_reflux` 的组内。

## 用途

- **`read[]`**：策略只读（电表/逆变器遥测等）。
- **`write[]`**：策略只写（限功率、启停等）。
- **`read_write[]`**：策略对虚拟 DO/AO **读 + 写回**（抑制倒计时秒数），用于运行时调试窗口。

## 动态防逆流开关（`DynamicAntiRefluxEnable`）

| JSON `key` | 宏名 |
|------------|------|
| `DynamicAntiRefluxEnable` | `ANTI_REFLUX_RW_KEY_DYNAMIC_CONTROL` |

### 数值语义（剩余抑制秒数 `S`）

- **`S == 0`**：动态侧**未抑制**——在总开关 `G` 与持久开关 `P_i` 均允许时，**参与**防逆流统筹。
- **`S != 0`**：动态侧**抑制中**——本逆变器**不参与**统筹（不写限功率、不计入额定分配等）；设备可照常运行，仅自动防逆流不干预。
- 上位机写 **`1800`** 表示：抑制自动调控约 **1800 秒**；策略进程按**策略周期**递减 `S` 并**写回**实时库，直至 **`S == 0`** 再恢复可统筹。

### 进程启动

- 防逆流策略进程**启动时**，对已绑定该虚拟点且 **`InverterRefs[].enable` 为真** 的逆变器，应将该点**初始化为 `0`**（写回实时库），表示**启动即非抑制状态**（避免沿用库内旧倒计时）。

### 与持久开关、总开关

- **总开关 `G`**：`strategy_control.json` → `config.system.enable`。关则整策略不输出。
- **持久开关 `P_i`**：**仅** `strategy_control.json` → **`InverterRefs[i].enable`**。为假则该机永不参与统筹；**动态开关无法越过**持久关。
- **设备模板**中 `strategy_points.group[].enabled`：**不参与业务**，防逆流逻辑**不解析**。

参与统筹条件：

```text
dynamic_bound = 已配置 read_write 中 DynamicAntiRefluxEnable 且绑定有效 slot（dblink 有效）
effective_on_i = G && P_i && ( !dynamic_bound || (S == 0) )
```

**未绑定动态虚拟点时**：不配置该项、`slot_index` 无效、`dblink_id` 无效等 —— **动态防逆流开关整体失效**（不读点、不写回、不倒计时、启动时也不对该点写 `0`）。该机是否参与统筹**仅由总开关 `G` 与持久开关 `P_i`（`InverterRefs[].enable`）决定**。

### 元素字段

与 `read[]` / `write[]` 单条相同，例如：

- `key`：`DynamicAntiRefluxEnable`
- `datattr`：与虚拟点一致（如 DO=3，AO=4）
- `slot_index`：该逆变器设备点表中的 slot
- `value_source`：一般为 `slot`

### 示例片段

```json
"read_write": [
  {
    "key": "DynamicAntiRefluxEnable",
    "datattr": 3,
    "slot_index": 11,
    "value_source": "slot"
  }
]
```
