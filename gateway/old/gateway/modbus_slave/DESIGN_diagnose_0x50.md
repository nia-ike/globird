# Modbus 0x50 DIAGNOSE — 实现说明（固件一期）

## 状态

| 项 | 内容 |
|----|------|
| 路径 | `ai/ref/out/app/modbus_slave/` |
| 功能码 | `0x50` `_FC_DIAGNOSE` |
| 一期已实现 | CAPS / NETIF / ROUTE / CONN_LIST / DNS / TCP_PROBE / JOB_* (ping) |
| 未实现 | RES_SUMMARY / JOB_LIST / pcap（已取消） |
| 上位机 | 抓包页已移除；连接列表 Grid + 过滤 |

## 文件

- `mb_diagnose.h` / `mb_diagnose.c` — 协议解析与执行
- `mb_protocol.h` — `_FC_DIAGNOSE`
- `mb_protocol_tcp.c` / `mb_protocol_rtu.c` — `mb_process_50H`

Makefile 自动收录 `.c`，无需改清单。

## 请求/应答头

- 请求头 4B：`SlaveAddr | 0x50 | Op | Flags`
- 应答头 8B：`SlaveAddr | 0x50 | Op | result | 0 | 0 | payload_len_be16` + payload

## 上位机

| 文件 | 说明 |
|------|------|
| `SmartGate/网络诊断/DiagnoseClient.cs` | 0x50 客户端 |
| `SmartGate/网络诊断/DiagnoseModels.cs` | DTO / Result |
| `SmartGate/窗口/Form网络测试.cs` | 已切到 0x50（去 SSH） |

工具栏「网络测试」已 `Visible=true`。
