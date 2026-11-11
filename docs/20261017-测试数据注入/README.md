# FCAS 半波测试数据注入

## 进程

- 二进制：`/ego/bin/fcas_inject`（随 IPK 升级下发，**不**进 monit）
- 默认输入：`/userdata/testdata/fcas_injection.csv`
- 依赖：`measure_process` / `db_process` 已运行

## 用法

```bash
/ego/bin/fcas_inject -o /tmp/fcas_inject.csv
/ego/bin/fcas_inject -T 5 -o /tmp/fcas_inject.csv   # 整表重复 5 遍，清屏显示进度
/ego/bin/fcas_inject -t 50 -T 5 -o /tmp/fcas_inject.csv   # 每行保持 50ms，默认 20ms
/ego/bin/fcas_inject -f /userdata/testdata/fcas_injection.csv -c 0 -o /tmp/fcas_inject.csv
/ego/bin/fcas_inject -C   # 清除注入，恢复 msr 全量发布
```

## 行为

1. 解析表头 `name=0|1` → `col_mask`，数据落入 `db_fcas_inject.data`
2. 每行保持 `-t` 毫秒（默认 20）后推进 `row_index`；msr 半波发布时对 mask=1 列从 `data[row_index]` 覆盖（权威源）
3. 播完 `state=HOLD`，停在最后一行。`-T n` 整表重复 n 遍后再 HOLD
4. `-T` 时清屏显示：周期、当前第几条/总条数、全程进度百分比
5. `-o` 为工程单位读回 CSV，含 `cycle` 列
