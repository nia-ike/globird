# Ser2Net Service

TCP服务器与串口双向数据转发服务

## 功能描述

Ser2Net服务是一个TCP服务器与串口之间的双向数据转发程序，支持：

- TCP服务器模式，监听指定端口
- 串口通信，支持多种波特率和串口参数配置
- 双向数据转发：TCP接收到的数据发送到串口，串口接收到的数据发送到TCP
- 多线程处理，确保数据转发的实时性
- 支持命令行参数配置
- 优雅的信号处理和资源清理

## 编译

```bash
cd app/ser2net_service
make clean
make
```

## 使用方法

### 基本用法

```bash
# 使用默认配置启动
./ser2net_service

# 指定TCP端口和串口设备
./ser2net_service -p 2000 -d /dev/ttyS0

# 完整参数配置
./ser2net_service -p 2000 -d /dev/ttyS0 -b 115200 -c 8 -y 0 -s 1
```

### 命令行参数

| 参数 | 说明 | 默认值 |
|------|------|--------|
| `-p <port>` | TCP服务器端口 | 2000 |
| `-d <device>` | 串口设备路径 | /dev/ttyS0 |
| `-b <baud>` | 波特率 | 115200 |
| `-c <data_bits>` | 数据位 | 8 |
| `-y <parity>` | 校验位 (0:无校验, 1:奇校验, 2:偶校验) | 0 |
| `-s <stop_bits>` | 停止位 | 1 |
| `-h` | 显示帮助信息 | - |

### 支持的波特率

- 9600
- 19200
- 38400
- 57600
- 115200
- 230400

### 支持的数据位

- 5
- 6
- 7
- 8

## 测试

### 编译测试程序

```bash
gcc -o test_ser2net test_ser2net.c
```

### 运行测试

1. 启动Ser2Net服务：
```bash
./ser2net_service -p 2000 -d /dev/ttyS0 -b 115200
```

2. 在另一个终端测试TCP连接：
```bash
./test_ser2net tcp
```

3. 测试串口通信：
```bash
./test_ser2net serial
```

4. 测试双向通信：
```bash
./test_ser2net both
```

## 工作原理

1. **初始化阶段**：
   - 解析命令行参数
   - 设置信号处理器
   - 初始化日志系统
   - 配置串口参数
   - 创建TCP服务器

2. **运行阶段**：
   - 创建两个独立线程：
     - TCP到串口转发线程
     - 串口到TCP转发线程
   - 等待TCP客户端连接
   - 实时转发数据

3. **数据转发**：
   - TCP接收到的数据立即发送到串口
   - 串口接收到的数据立即发送到TCP客户端
   - 使用非阻塞I/O确保实时性

4. **错误处理**：
   - 客户端断开连接时自动重新等待连接
   - 串口错误时记录日志但不中断服务
   - 支持优雅退出（Ctrl+C）

## 信号处理

程序支持以下信号：
- `SIGINT` (Ctrl+C)
- `SIGTERM`
- `SIGQUIT`

收到这些信号时，程序会：
1. 设置停止标志
2. 等待线程结束
3. 关闭所有文件描述符
4. 清理资源
5. 退出程序

## 日志输出

程序运行时会输出以下信息：
- 配置参数
- 连接状态
- 数据转发统计
- 错误信息

示例输出：
```
Ser2Net Process Starting...
Serial port /dev/ttyS0 configured: 115200-8-0-1
TCP server listening on port 2000
Ser2Net Process started successfully
Configuration:
  TCP Port: 2000
  Serial Device: /dev/ttyS0
  Baud Rate: 115200
  Data Bits: 8
  Parity: 0
  Stop Bits: 1
Press Ctrl+C to stop
Client connected from 127.0.0.1:12345
TCP->Serial: 20 bytes
Serial->TCP: 18 bytes
```

## 注意事项

1. **权限要求**：需要串口设备的读写权限
2. **设备存在性**：确保指定的串口设备存在
3. **端口占用**：确保指定的TCP端口未被占用
4. **硬件流控**：程序禁用了硬件流控，如需启用请修改代码
5. **缓冲区大小**：默认缓冲区大小为1024字节，可根据需要调整

## 故障排除

### 常见问题

1. **无法打开串口设备**
   - 检查设备路径是否正确
   - 确认用户权限
   - 检查设备是否被其他程序占用

2. **TCP端口被占用**
   - 使用 `netstat -tuln` 检查端口占用情况
   - 选择其他可用端口

3. **数据转发异常**
   - 检查串口参数配置
   - 确认波特率匹配
   - 查看日志输出

### 调试模式

可以通过修改代码中的调试输出级别来获取更详细的运行信息。

## 许可证

本项目遵循原有代码的许可证。 