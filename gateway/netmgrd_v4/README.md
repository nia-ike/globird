# netmgrd_v4

`tmp/netmgrd_v3` 的设计与实现已弃用。本目录为 **全新工程**，仅复用经产品验证过的**思路**（如：每拍顺序、单一路径去抖动、适配器幂等启动），不复制其代码与目录历史。

## 本目录提供什么

- 最小可构建骨架（C11）
- `DESIGN.md`：v4 的架构边界与未决点，便于迭代
- 可逐步替换/填充 `src/`，不强行一次写完

## 构建

**默认**在装有 Buildroot 交叉链的机器上编出 **ARM uClibc** 可执行文件（与 v3 相同 `CC` / `SYSROOT`）。链接成功后会用 `file` 检查是否为 ARM；若你曾用本机 `gcc` 编过，务必先 **`make clean`** 再编，否则会混进 x86 的 `.o`。

```bash
cd tmp/netmgrd_v4
make clean && make              # ARM 目标（勿在板子上执行 make，在开发机交叉编）
# 仅在 x86 开发机自测、不拷到板子：  make clean && make HOST=1
```

不要用已废弃的 `make CC=gcc SYSROOT=`；请改用 **`HOST=1`**。

## 适配器手测（WiFi / PPP）

与守护主循环无关，一次只执行一种动作（返回后进程退出）。

**默认**从 `/ech/bin/netmgr_v3.json` 读第一个 `wifi_sta` / `ppp` 的 `conn`（`ssid`/`psk`、`device`/`apn` 等），执行前、后会在标准输出打印配置路径、参数与 `OK`/`FAIL`：

```text
./build/bin/netmgrd_v4 -w
./build/bin/netmgrd_v4 -p
./build/bin/netmgrd_v4 -C /other/path.json -w
```

仍可用命令行**完全手写**或**只覆盖**部分字段，例如：`-f` + `-s/-k` 为手写 WiFi；`-D` 且 `-a` 为手写 PPP。`-W` / `-P` 无 `-i` 时从 JSON 取 `ifname`。

## 守护模式（方案 B）

`-M` 启动守护主循环，采用“连接保活与路由决策解耦”：

- `wlan0` / `ppp0` 由 `LinkKeeper` 周期保活（调用 adapter，**不改 default**）
- 探测 + 去抖产出三比特（eth/wlan/ppp）
- `R` 策略层决定 `want`，并由路由层统一下发 default（唯一写路由）
- 触发条件：`bits` 变化，或 `want != have` 且非 pending
- `--managed-view table` 在 TTY 下固定区域重绘；并自动静默 adapter 外部命令输出，避免冲乱面板
- 守护运行时会写 `/var/run/netmgr/status.json`（原子替换）
- 最小自愈：manager 会按 `eth0->wlan0->ppp0` 逐个候选下发临时 `/32` 探测路由（不切 default）后探测，并做轻量去抖（up/down 连续阈值=2）

```text
./build/bin/netmgrd_v4 -M -d
./build/bin/netmgrd_v4 -M -C /ech/bin/netmgr_v3.json
./build/bin/netmgrd_v4 -M --managed-view table -C /ech/bin/netmgr_v3.json
```

## 各模块单独验证参数

除 `-w/-W/-p/-P/-N/-M` 外，新增以下“模块级”入口，便于单独联调：

```text
./build/bin/netmgrd_v4 --policy-bits 101
./build/bin/netmgrd_v4 --route-get
./build/bin/netmgrd_v4 --route-apply eth0
./build/bin/netmgrd_v4 --route-apply none
./build/bin/netmgrd_v4 --linkkeeper-once -C /ech/bin/netmgr_v3.json
./build/bin/netmgrd_v4 --linkkeeper-run -d -C /ech/bin/netmgr_v3.json
./build/bin/netmgrd_v4 --linkkeeper-run --linkkeeper-interval 2000 -d
./build/bin/netmgrd_v4 --linkkeeper-run --lk-wifi-ms 20000 --lk-ppp-ms 30000 -d
./build/bin/netmgrd_v4 --linkkeeper-run --linkkeeper-view table --linkkeeper-interval 1000
./build/bin/netmgrd_v4 --gateway eth0
```

说明：
- `--policy-bits XYZ`：`X/Y/Z` 分别对应 `eth/wlan/ppp` 的 0/1，输出 `R` 与 `want`
- `--route-get|--route-apply`：仅验证路由模块（唯一写 default 的模块）
- `--linkkeeper-once`：执行一次连接保活 tick（不进入守护循环）
- `--linkkeeper-run`：持续执行连接保活 tick（默认 1000ms，支持 `--linkkeeper-interval MS`）
- `--lk-wifi-ms` / `--lk-ppp-ms`：配置 linkkeeper 保活周期（默认 `10000` / `15000` ms；对 `-M` 和 `--linkkeeper-run` 都生效）
- `--linkkeeper-view table|log`：`linkkeeper-run` 输出模式（`table` 为固定表格刷新，不滚屏）
- `--gateway IF`：验证网关解析链路（run 文件 -> netlink -> network.json）

## 与 v3 的关系

- **无** 代码依赖、无 git 子树引用 v3
- 若从设备上已部署 v3 二进制，可并行使用不同可执行名（`netmgrd` vs `netmgrd_v4`）与不同配置路径，部署策略由你方决定



## 手工备忘
wifi 和ppp的调用和守护已经交给了netmgrd_v4了
1. 删除原来的monit 脚本关于 wifi 和ppp的
2. 删除ech_prepare 和 ech_process 相关的 wifi ppp
3. 检查env 脚本是否有 wifi 和ppp的
