# 🔍 My Traceroute - 路径跟踪工具

一个用 C++ 编写的极简 Traceroute 实现，用于学习网络诊断和底层协议编程。

## 📖 项目简介

Traceroute（路由跟踪）是一个网络诊断工具，用于显示数据包从源主机到目标主机所经过的路由器路径。本项目通过底层套接字编程实现了 Traceroute 的核心功能。

### 🎯 核心原理

1. **TTL (Time-To-Live) 机制**
   - 每个 IP 数据包都有一个 TTL 字段，初始值由发送者设定
   - 数据包每经过一个路由器，TTL 减 1
   - 当 TTL 减到 0 时，路由器会丢弃该数据包并返回 ICMP "Time Exceeded" 消息

2. **探测过程**
   - 程序从 TTL=1 开始，逐步递增到 TTL=30
   - 对于每个 TTL 值，发送一个 UDP 数据包到目标主机的不可达端口
   - 接收中间路由器返回的 ICMP 响应，从中提取路由器的 IP 地址
   - 当到达目标主机时，会收到 ICMP "Destination Unreachable" 消息

3. **涉及的协议**
   - **UDP (用户数据报协议)**: 用于发送探测包
   - **ICMP (互联网控制消息协议)**: 用于接收路由器和目标主机的响应
   - **IP (互联网协议)**: TTL 字段的载体

## 🛠️ 技术要点

### 关键技术实现

| 技术点 | 说明 |
|--------|------|
| **域名解析** | 使用 `gethostbyname()` 将主机名转换为 IP 地址 |
| **UDP Socket** | 创建 `SOCK_DGRAM` 套接字发送探测包 |
| **TTL 设置** | 通过 `setsockopt(IP_TTL)` 设置数据包生存时间 |
| **Raw Socket** | 创建 `SOCK_RAW` 套接字接收 ICMP 响应（需要 root 权限）|
| **超时处理** | 通过 `SO_RCVTIMEO` 设置接收超时，处理无响应的路由器 |
| **ICMP 解析** | 解析 IP 头部和 ICMP 头部，识别响应类型 |

### ICMP 响应类型

- **Type 11 (Time Exceeded)**: 中间路由器返回，表示 TTL 已耗尽
- **Type 3 (Destination Unreachable)**: 目标主机返回，表示已到达终点

## 📋 系统要求

- **操作系统**: Linux (支持 POSIX 套接字)
- **编译器**: g++ (支持 C++11 及以上)
- **权限**: 需要 root (sudo) 权限运行

## 🚀 快速开始

### 编译程序

```bash
cd Traceroute
make
```

### 运行程序

```bash
# 基本用法
sudo ./my_traceroute <目标主机>

# 示例：追踪到 Google 的路径
sudo ./my_traceroute google.com

# 示例：追踪到百度的路径
sudo ./my_traceroute baidu.com

# 示例：追踪到指定 IP
sudo ./my_traceroute 8.8.8.8
```

### 使用 Makefile 快捷运行

```bash
# 编译并运行
make run HOST=google.com
```

### 清理编译文件

```bash
make clean
```

## 📊 输出示例

```
🎯 目标主机: google.com (142.250.185.46)
📊 最大跳数: 30 跳
⏱️  超时时间: 3 秒

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
开始路径跟踪...

1       192.168.1.1 (router.local)
2       10.0.0.1 (gateway.isp.com)
3       203.0.113.5 (core-router-1.isp.com)
4       * * * (请求超时)
5       142.250.185.46 (google.com) [目标已到达]

✅ 成功到达目标主机！
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

## 🔧 常见问题

### 1. 权限不足错误

```
❌ 无法创建 ICMP Raw Socket: Operation not permitted
   💡 提示：此程序需要 root 权限，请使用 sudo 运行！
```

**解决方法**: 使用 `sudo` 运行程序

### 2. 某些跳显示超时

```
4       * * * (请求超时)
```

**原因**: 某些路由器配置了防火墙规则，不响应 ICMP 请求

### 3. 无法解析主机名

```
❌ 无法解析主机名: unknownhost.com
```

**解决方法**: 检查主机名拼写或网络连接

## 📚 学习资源

### 相关协议文档

- [RFC 792 - ICMP 协议](https://tools.ietf.org/html/rfc792)
- [RFC 791 - IP 协议](https://tools.ietf.org/html/rfc791)
- [RFC 768 - UDP 协议](https://tools.ietf.org/html/rfc768)

### 推荐阅读

- 《TCP/IP 详解 卷1：协议》- W. Richard Stevens
- 《Unix 网络编程 卷1：套接字联网 API》- W. Richard Stevens

## 🔍 代码结构

```
my_traceroute.cpp
├── main()                      # 主函数
│   ├── resolve_hostname()      # 域名解析
│   └── for (ttl = 1; ttl <= MAX_HOPS; ttl++)
│       ├── send_probe_packet() # 发送 UDP 探测包
│       │   ├── socket(SOCK_DGRAM)
│       │   ├── setsockopt(IP_TTL)  # 设置 TTL
│       │   └── sendto()
│       └── receive_icmp_response() # 接收 ICMP 响应
│           ├── socket(SOCK_RAW, IPPROTO_ICMP)
│           ├── setsockopt(SO_RCVTIMEO)  # 设置超时
│           ├── recvfrom()
│           └── 解析 ICMP 包
│               ├── Type 11: Time Exceeded
│               └── Type 3: Destination Unreachable
```

## ⚙️ 配置参数

在 `my_traceroute.cpp` 中可以修改以下常量：

| 参数 | 默认值 | 说明 |
|------|--------|------|
| `MAX_HOPS` | 30 | 最大跳数 |
| `RECV_TIMEOUT_SEC` | 3 | 接收超时时间（秒）|
| `UDP_BASE_PORT` | 33434 | UDP 探测包目标端口 |
| `PACKET_SIZE` | 64 | 数据包大小（字节）|

## 🎓 学习重点

通过这个项目，你将学习到：

1. ✅ **原始套接字 (Raw Socket)** 的创建和使用
2. ✅ **套接字选项 (setsockopt)** 的设置
3. ✅ **IP 协议头部** 的结构和 TTL 机制
4. ✅ **ICMP 协议** 的工作原理和消息类型
5. ✅ **网络字节序** 和地址转换
6. ✅ **超时处理** 和错误处理
7. ✅ **底层网络编程** 的实践技巧

## ⚠️ 注意事项

1. **权限要求**: 程序必须以 root 权限运行才能创建原始套接字
2. **防火墙**: 某些网络环境下，ICMP 可能被防火墙拦截
3. **延迟**: 反向 DNS 查询可能导致程序运行缓慢
4. **准确性**: 由于网络路由的动态性，不同时间的结果可能不同

## 📝 许可证

本项目仅用于教育和学习目的。

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

---

**Happy Networking! 🌐**
