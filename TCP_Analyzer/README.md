# 📡 TCP 协议分析器 - 有状态连接跟踪器

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue.svg)
![Platform](https://img.shields.io/badge/platform-Linux-green.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B-orange.svg)
![License](https://img.shields.io/badge/license-MIT-purple.svg)

**一个基于 Linux AF_PACKET 原始套接字的有状态 TCP 协议分析器**

极简版 tcpdump · 连接跟踪 · 状态机实现

</div>

---

## 📋 目录

- [项目简介](#-项目简介)
- [核心特性](#-核心特性)
- [技术原理](#-技术原理)
- [快速开始](#-快速开始)
- [使用方法](#-使用方法)
- [输出示例](#-输出示例)
- [技术细节](#-技术细节)
- [常见问题](#-常见问题)
- [许可证](#-许可证)

---

## 🎯 项目简介

这是一个**有状态的 TCP 协议分析器**，类似于简化版的 `tcpdump`，但专注于 TCP 连接的**状态跟踪**。它能够：

- 🔍 捕获网络接口上的所有 TCP 数据包
- 🔄 跟踪每个 TCP 连接的完整生命周期
- 📊 实时显示连接的状态转换（三次握手、数据传输、四次挥手）
- 🎨 使用彩色 emoji 标记不同类型的事件

### 🆚 与简单 Packet Sniffer 的区别

| 特性 | Packet Sniffer | TCP Analyzer |
|------|----------------|--------------|
| **难度** | ⭐⭐ 基础 | ⭐⭐⭐⭐⭐ 高级 |
| **状态管理** | ❌ 无状态 | ✅ 有状态 |
| **连接跟踪** | ❌ 每个包独立 | ✅ 按连接组织 |
| **状态机** | ❌ 无 | ✅ 完整 TCP 状态机 |
| **双向识别** | ❌ 无 | ✅ 规范化连接 ID |

---

## ✨ 核心特性

### 🔐 协议层次解析

```
┌─────────────────────────────────────┐
│  Layer 2: Ethernet (以太网)          │  ← 14 字节
├─────────────────────────────────────┤
│  Layer 3: IPv4 (IP 头部)             │  ← 20+ 字节
├─────────────────────────────────────┤
│  Layer 4: TCP (TCP 头部)             │  ← 20+ 字节
├─────────────────────────────────────┤
│  Layer 7: Data (应用数据)            │  ← 可变长度
└─────────────────────────────────────┘
```

### 🔄 TCP 状态机实现

本项目实现了完整的 TCP 状态机：

```
三次握手 (连接建立)：
  CLOSED → [SYN] → SYN_SENT → [SYN-ACK] → ESTABLISHED

数据传输：
  ESTABLISHED → [DATA] → ESTABLISHED

四次挥手 (连接关闭)：
  主动关闭方：
    ESTABLISHED → [FIN] → FIN_WAIT_1 → [ACK] → FIN_WAIT_2
    → [FIN] → TIME_WAIT → [ACK] → CLOSED

  被动关闭方：
    ESTABLISHED → [FIN] → CLOSE_WAIT → [FIN] → LAST_ACK
    → [ACK] → CLOSED

特殊情况：
  同时关闭：FIN_WAIT_1 → [FIN] → CLOSING → [ACK] → CLOSED
  连接重置：任何状态 → [RST] → CLOSED
```

### 🎨 彩色事件标记

- 🟢 **绿色**：连接建立事件（SYN, SYN-ACK, ACK）
- 📦 **蓝色盒子**：数据传输事件
- 🔵 **蓝色圆圈**：连接关闭事件（FIN, ACK）
- 🔴 **红色**：连接重置（RST）

### 🔑 连接规范化 (Canonicalization)

关键技术点：TCP 连接是**双向**的，(A → B) 和 (B → A) 应该被视为**同一个连接**。

```cpp
// 问题：如何确保这两个数据包映射到同一个连接？
包1: 192.168.1.100:8080 → 10.0.0.1:80
包2: 10.0.0.1:80 → 192.168.1.100:8080

// 解决方案：规范化 (Canonicalization)
// 总是将较小的 IP 作为 src_ip，如果 IP 相同则比较端口号
规范化后的 ID: 10.0.0.1:80 ↔ 192.168.1.100:8080
```

---

## 🚀 快速开始

### 📦 系统要求

- **操作系统**：Linux (内核 2.2+)
- **编译器**：g++ (支持 C++11)
- **权限**：需要 root (sudo) 权限

### 🛠️ 编译

```bash
# 进入项目目录
cd TCP_Analyzer

# 编译程序
make

# 或者手动编译
g++ -Wall -Wextra -std=c++11 -O2 -o tcp_analyzer tcp_analyzer.cpp
```

---

## 📖 使用方法

### 基本用法

```bash
# 查看所有网络接口
ip link show

# 监听指定接口（需要 sudo 权限）
sudo ./tcp_analyzer eth0

# 监听无线网卡
sudo ./tcp_analyzer wlan0

# 监听本地回环接口（用于本地测试）
sudo ./tcp_analyzer lo
```

### 使用 Makefile

```bash
# 编译
make

# 运行（指定接口）
make run INTERFACE=eth0

# 清理编译产物
make clean

# 显示帮助
make help
```

### 测试示例

在一个终端运行分析器：
```bash
sudo ./tcp_analyzer lo
```

在另一个终端生成 TCP 流量：
```bash
# 测试 1: 简单的 HTTP 请求
curl http://127.0.0.1:80

# 测试 2: 使用 netcat 建立连接
# 终端 1：启动服务器
nc -l 8888

# 终端 2：连接到服务器
nc 127.0.0.1 8888
```

---

## 📊 输出示例

```
====================================================
      TCP 协议分析器 - 有状态连接跟踪器
====================================================
监听接口: lo
开始时间: 1700000000.000
====================================================

✅ 套接字创建成功，开始捕获数据包...

[0.000] 🟢 新连接发起 (SYN): 127.0.0.1:45678 -> 127.0.0.1:80 [CLOSED -> SYN_SENT]
[0.001] 🟢 连接建立 (SYN-ACK): 127.0.0.1:80 <-> 127.0.0.1:45678 [SYN_SENT -> ESTABLISHED]
[0.002] 🟢 连接确认 (ACK): 127.0.0.1:45678 <-> 127.0.0.1:80 [SYN_SENT -> ESTABLISHED]

[0.010] 📦 数据传输: 127.0.0.1:45678 -> 127.0.0.1:80 (256 bytes) [ESTABLISHED]
[0.012] 📦 数据传输: 127.0.0.1:80 -> 127.0.0.1:45678 (1024 bytes) [ESTABLISHED]

[0.100] 🔵 连接关闭发起 (FIN): 127.0.0.1:45678 -> 127.0.0.1:80 [ESTABLISHED -> FIN_WAIT_1]
[0.101] 🔵 关闭确认 (ACK): 127.0.0.1:80 <-> 127.0.0.1:45678 [FIN_WAIT_1 -> FIN_WAIT_2]
[0.102] 🔵 对方关闭 (FIN): 127.0.0.1:80 <-> 127.0.0.1:45678 [FIN_WAIT_2 -> TIME_WAIT]
[0.103] 🔵 连接完全关闭 (ACK): 127.0.0.1:45678 <-> 127.0.0.1:80 [TIME_WAIT -> CLOSED]
```

### 输出字段说明

```
[时间戳] 事件类型: 源地址:端口 方向 目标地址:端口 (详情) [状态转换]
```

- **时间戳**：相对于程序启动的时间（秒）
- **事件类型**：🟢=建立 📦=数据 🔵=关闭 🔴=重置
- **方向**：`->` 单向, `<->` 双向
- **详情**：数据包类型（SYN/ACK/FIN/RST）或数据大小
- **状态转换**：`[当前状态 -> 新状态]`

---

## 🔬 技术细节

### 核心数据结构

#### 1. 连接标识符 (ConnectionID)

```cpp
struct ConnectionID {
    uint32_t src_ip;     // 源 IP 地址
    uint32_t dst_ip;     // 目标 IP 地址
    uint16_t src_port;   // 源端口号
    uint16_t dst_port;   // 目标端口号
};
```

#### 2. 连接跟踪表

```cpp
std::map<ConnectionID, TcpState> connection_tracker;
```

**作用**：
- **Key**: 规范化的 ConnectionID（确保双向数据包映射到同一连接）
- **Value**: 当前的 TCP 状态
- **功能**: 记录每个连接的状态，根据接收到的 TCP 标志位更新状态

### TCP 标志位解析

```cpp
struct tcphdr {
    // ... 其他字段 ...
    unsigned char  fin:1;  // FIN: 结束连接
    unsigned char  syn:1;  // SYN: 同步序列号（建立连接）
    unsigned char  rst:1;  // RST: 重置连接
    unsigned char  psh:1;  // PSH: 推送数据
    unsigned char  ack:1;  // ACK: 确认号有效
    unsigned char  urg:1;  // URG: 紧急指针有效
};
```

### 状态转换逻辑示例

```cpp
// 示例：处理三次握手的第一步
if (current_state == CLOSED && tcp->syn && !tcp->ack) {
    connection_tracker[key] = SYN_SENT;
    printf("新连接发起 (SYN)\n");
}

// 示例：处理数据传输
if (current_state == ESTABLISHED && data_len > 0) {
    printf("数据传输 (%d bytes)\n", data_len);
}
```

### AF_PACKET 套接字

```cpp
// 创建原始套接字
int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

// 绑定到指定网络接口
bind(sock, (struct sockaddr*)&sll, sizeof(sll));

// 接收数据包
recv(sock, buffer, sizeof(buffer), 0);
```

**优势**：
- 工作在数据链路层，可以捕获完整的以太网帧
- 无需其他依赖库（如 libpcap）
- 高性能，低延迟

---

## ❓ 常见问题

### Q1: 为什么需要 sudo 权限？

**A**: AF_PACKET 原始套接字需要 `CAP_NET_RAW` 能力，这是一种特权操作。只有 root 用户或具有该能力的进程才能创建原始套接字。

```bash
# 方法 1：使用 sudo 运行
sudo ./tcp_analyzer eth0

# 方法 2：给程序添加能力（不推荐）
sudo setcap cap_net_raw+ep ./tcp_analyzer
./tcp_analyzer eth0
```

### Q2: 如何查看可用的网络接口？

```bash
# 方法 1：使用 ip 命令
ip link show

# 方法 2：使用 ifconfig
ifconfig -a

# 方法 3：查看 /sys/class/net
ls /sys/class/net/
```

### Q3: 为什么看不到任何输出？

可能的原因：
1. **接口没有流量**：尝试在另一个终端生成流量（如 `ping`, `curl`）
2. **错误的接口名**：确认接口名称正确
3. **只显示 TCP 流量**：程序会过滤掉非 TCP 数据包（如 UDP, ICMP）

### Q4: 如何停止程序？

按 `Ctrl + C` 发送 SIGINT 信号即可停止程序。

### Q5: 状态机是否完整？

这是一个**简化的 TCP 状态机**，实现了主要的状态转换。完整的 TCP 状态机有 11 个状态，本项目实现了最常见的场景：
- ✅ 三次握手（SYN, SYN-ACK, ACK）
- ✅ 数据传输（ESTABLISHED）
- ✅ 四次挥手（FIN, ACK, FIN, ACK）
- ✅ 连接重置（RST）
- ✅ 同时关闭（CLOSING）

未实现的部分：
- ❌ 半关闭状态的精细处理
- ❌ 超时重传检测
- ❌ 连接超时自动清理

---

## 🔧 扩展建议

如果你想进一步增强这个项目，可以考虑：

1. **📈 统计信息**
   - 连接总数
   - 数据传输总量
   - 平均连接时长
   - 连接失败率

2. **💾 数据导出**
   - 导出为 PCAP 格式
   - JSON 格式的连接日志
   - CSV 格式的统计报告

3. **🎨 可视化**
   - 实时连接图
   - 状态转换时间线
   - 流量热力图

4. **🔍 过滤功能**
   - 按 IP 地址过滤
   - 按端口号过滤
   - 按连接状态过滤

5. **⚡ 性能优化**
   - 多线程处理
   - 零拷贝优化
   - 连接表自动清理

6. **🛡️ 安全检测**
   - SYN Flood 检测
   - 端口扫描检测
   - 异常连接模式识别

---

## 📚 参考资料

- **RFC 793**: TCP Protocol Specification
- **RFC 9293**: Transmission Control Protocol (TCP) - 最新版本
- **Linux Packet(7) Manual**: AF_PACKET 套接字文档
- **TCP/IP Illustrated, Volume 1**: 经典的 TCP/IP 协议详解

---

## 📄 许可证

本项目采用 MIT 许可证。详见 [LICENSE](../LICENSE) 文件。

---

## 🙏 致谢

本项目基于：
- Linux 内核的 AF_PACKET 套接字接口
- C++ 标准库的 STL 容器

---

<div align="center">

**🌟 如果这个项目对你有帮助，请给一个 Star！**

Made with ❤️ by C++ Network Security Expert

</div>
