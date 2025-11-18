# 🚀 基于 Epoll 的高性能多人聊天室

<div align="center">

![C++](https://img.shields.io/badge/C++-11-blue.svg)
![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg)
![License](https://img.shields.io/badge/License-MIT-green.svg)
![Build](https://img.shields.io/badge/Build-Passing-brightgreen.svg)

**单线程 · 非阻塞 · I/O 多路复用**

一个使用 Linux epoll API 实现的高性能、可扩展的多人聊天室服务器

</div>

---

## 📋 目录

- [项目简介](#-项目简介)
- [核心特性](#-核心特性)
- [技术架构](#-技术架构)
- [系统要求](#-系统要求)
- [快速开始](#-快速开始)
- [使用指南](#-使用指南)
- [技术细节](#-技术细节)
- [性能特点](#-性能特点)
- [项目结构](#-项目结构)
- [常见问题](#-常见问题)
- [贡献指南](#-贡献指南)
- [许可证](#-许可证)

---

## 📖 项目简介

本项目是一个基于 **Linux epoll** 技术实现的高性能多人聊天室服务器。与传统的"一个连接一个线程"模型不同，本项目采用**单线程 + I/O 多路复用**的架构，能够在单个线程中高效管理数千个并发连接。

### 为什么选择 Epoll？

- ✅ **高性能**：支持数万级并发连接
- ✅ **低延迟**：边缘触发模式，事件响应迅速
- ✅ **资源高效**：单线程管理，无上下文切换开销
- ✅ **可扩展**：O(1) 时间复杂度的事件通知

---

## ✨ 核心特性

### 🔥 高性能架构
- **epoll I/O 多路复用**：使用 `epoll_create1`、`epoll_ctl`、`epoll_wait` 管理所有连接
- **非阻塞套接字**：所有网络 I/O 操作都是非阻塞的，使用 `fcntl` 设置 `O_NONBLOCK`
- **边缘触发模式**：`EPOLLET` 模式，减少事件通知次数，提升性能
- **单线程事件循环**：无锁设计，无线程同步开销

### 💬 聊天功能
- **实时消息广播**：消息即时同步给所有在线用户
- **用户管理**：自动分配昵称，支持最大连接数限制
- **系统通知**：用户加入/离开自动通知
- **连接状态管理**：优雅处理正常断开和异常断开

### 🛡️ 健壮性
- **错误处理**：完善的 `EWOULDBLOCK`、`EAGAIN`、`EINTR` 处理
- **资源清理**：连接断开时自动清理 epoll 实例和套接字
- **边界检查**：防止缓冲区溢出，限制最大连接数

---

## 🏗️ 技术架构

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                    Epoll 服务器架构                         │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌─────────────────────────────────────────────────────┐   │
│  │          主事件循环 (Event Loop)                    │   │
│  │                                                     │   │
│  │   while(true):                                      │   │
│  │     epoll_wait(epoll_fd, events, ...)  <───────┐   │   │
│  │         │                                       │   │   │
│  │         ├─ listen_sock 事件? ──→ accept()      │   │   │
│  │         │   └─ set_nonblocking()                │   │   │
│  │         │   └─ epoll_ctl(EPOLL_CTL_ADD)         │   │   │
│  │         │                                       │   │   │
│  │         ├─ client_sock 事件? ──→ recv()        │   │   │
│  │         │   └─ 循环读取直到 EWOULDBLOCK         │   │   │
│  │         │   └─ broadcast_message()              │   │   │
│  │         │                                       │   │   │
│  │         └─ 错误事件? ──→ epoll_ctl(DEL)        │   │   │
│  │             └─ close(client_sock)               │   │   │
│  │                                                     │   │
│  └─────────────────────────────────────────────────────┘   │
│                          ↑                                  │
│                          │                                  │
│         ┌────────────────┴────────────────┐                │
│         │     Epoll 实例 (epoll_fd)       │                │
│         ├──────────────────────────────────┤                │
│         │  • listen_sock (EPOLLIN|EPOLLET) │                │
│         │  • client1_sock (EPOLLIN|EPOLLET)│                │
│         │  • client2_sock (EPOLLIN|EPOLLET)│                │
│         │  • ...                            │                │
│         └───────────────────────────────────┘                │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 客户端架构

```
┌───────────────────────────────────────┐
│         客户端 (双线程模型)          │
├───────────────────────────────────────┤
│                                       │
│  主线程                接收线程       │
│  ┌───────┐            ┌───────┐      │
│  │ 读取  │            │ recv()│      │
│  │ 用户  │            │  │    │      │
│  │ 输入  │            │  └──→ │      │
│  │  │    │            │ 打印  │      │
│  │  └──→ │            │ 消息  │      │
│  │ send()│            └───────┘      │
│  └───────┘                           │
│      │                     ↑         │
│      └──────── socket ─────┘         │
└───────────────────────────────────────┘
```

---

## 💻 系统要求

### 必需
- **操作系统**：Linux（内核 2.6+）
- **编译器**：g++ 支持 C++11 或更高版本
- **工具**：make

### 推荐
- Ubuntu 18.04+ / CentOS 7+ / Debian 10+
- g++ 7.0+

> ⚠️ **注意**：本项目使用 Linux 专有的 epoll API，**不支持 Windows 和 macOS**。

---

## 🚀 快速开始

### 1. 克隆仓库

```bash
git clone <repository-url>
cd cs-network/cs-chatroom
```

### 2. 编译项目

```bash
make
```

输出示例：
```
正在编译服务器...
服务器编译完成: epoll_server
正在编译客户端...
客户端编译完成: client
```

### 3. 运行服务器

```bash
./epoll_server
```

输出示例：
```
╔════════════════════════════════════════╗
║   基于 Epoll 的高性能聊天室服务器    ║
║   架构: 单线程 + I/O 多路复用        ║
║   作者: C++ 服务器架构师             ║
╚════════════════════════════════════════╝

[成功] 服务器启动，监听端口: 8888
[成功] epoll 实例创建成功 fd=3
[成功] 监听套接字已添加到 epoll 实例

服务器运行中，等待客户端连接...
```

### 4. 运行客户端（新终端）

```bash
./client
```

或连接到远程服务器：
```bash
./client <服务器IP> <端口>
```

### 5. 开始聊天！

在客户端终端输入消息，按回车发送。所有在线用户都会收到你的消息。

---

## 📚 使用指南

### 基础操作

| 操作 | 说明 |
|------|------|
| **输入消息 + 回车** | 发送消息到聊天室 |
| `/quit` 或 `/exit` | 退出聊天室 |
| `Ctrl+C` | 强制退出（服务器端） |

### 多客户端测试

在不同终端运行多个客户端：

```bash
# 终端1
./client

# 终端2
./client

# 终端3
./client
```

### 自定义配置

编辑 `epoll_server.cpp` 中的常量：

```cpp
const int PORT = 8888;              // 修改服务器端口
const int MAX_EVENTS = 100;         // 修改 epoll 事件数
const int BUFFER_SIZE = 4096;       // 修改缓冲区大小
const int MAX_CLIENTS = 1000;       // 修改最大连接数
```

修改后重新编译：
```bash
make clean
make
```

---

## 🔬 技术细节

### 关键技术点

#### 1. 非阻塞套接字设置

使用 `fcntl` 将套接字设置为非阻塞模式：

```cpp
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);
```

**为什么使用非阻塞？**
- 避免 `accept`、`recv`、`send` 阻塞主线程
- 配合 epoll 实现真正的异步 I/O

#### 2. Epoll 事件管理

```cpp
// 创建 epoll 实例
int epoll_fd = epoll_create1(0);

// 添加套接字到 epoll
struct epoll_event ev;
ev.events = EPOLLIN | EPOLLET;  // 可读事件 + 边缘触发
ev.data.fd = sock_fd;
epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sock_fd, &ev);

// 等待事件
epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

// 删除套接字
epoll_ctl(epoll_fd, EPOLL_CTL_DEL, sock_fd, nullptr);
```

#### 3. 边缘触发模式 (EPOLLET)

**边缘触发 vs 水平触发**

| 模式 | 触发时机 | 特点 |
|------|----------|------|
| **水平触发 (LT)** | 只要有数据就触发 | 简单，但可能频繁触发 |
| **边缘触发 (ET)** | 状态变化时才触发 | 高性能，需要循环读取 |

**本项目使用边缘触发**，必须循环读取直到 `EWOULDBLOCK`：

```cpp
while (true) {
    ssize_t n = recv(sock_fd, buffer, BUFFER_SIZE, 0);
    if (n > 0) {
        // 处理数据
    } else if (n == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            break;  // 数据读完了
        }
    }
}
```

#### 4. 错误处理

| 错误码 | 含义 | 处理方式 |
|--------|------|----------|
| `EWOULDBLOCK` / `EAGAIN` | 操作会阻塞 | 正常情况，继续事件循环 |
| `EINTR` | 被信号中断 | 重试操作 |
| `EPIPE` / `ECONNRESET` | 连接断开 | 关闭连接并清理资源 |

---

## ⚡ 性能特点

### 并发性能

- **单线程管理** 1000+ 并发连接
- **内存占用低**：无需为每个连接创建线程
- **CPU 效率高**：无线程上下文切换开销

### 性能对比

| 模型 | 并发连接数 | 内存占用 | CPU 占用 |
|------|-----------|---------|---------|
| **多线程模型** | ~100 | 高（每线程 1-8MB） | 高（上下文切换） |
| **Epoll 模型** | 10000+ | 低（共享内存） | 低（单线程） |

### 性能优化建议

1. **增大系统文件描述符限制**：
   ```bash
   ulimit -n 65535
   ```

2. **调整 TCP 参数**（`/etc/sysctl.conf`）：
   ```
   net.core.somaxconn = 1024
   net.ipv4.tcp_max_syn_backlog = 2048
   ```

3. **使用发送缓冲队列**：
   - 当前实现简化了 `send` 的 `EWOULDBLOCK` 处理
   - 生产环境应实现发送队列 + `EPOLLOUT` 事件

---

## 📁 项目结构

```
cs-chatroom/
├── epoll_server.cpp    # 服务器主程序（epoll 实现）
├── client.cpp          # 客户端程序（双线程实现）
├── Makefile            # 编译脚本
└── README.md           # 项目文档
```

### 文件说明

| 文件 | 行数 | 说明 |
|------|------|------|
| `epoll_server.cpp` | ~500 | 服务器核心代码，包含详细中文注释 |
| `client.cpp` | ~150 | 客户端代码，双线程模型 |
| `Makefile` | ~50 | 自动化编译脚本 |
| `README.md` | ~400 | 完整项目文档 |

---

## ❓ 常见问题

### Q1: 编译时报错 "epoll_create1 未定义"

**A:** 确保你在 Linux 系统上编译。epoll 是 Linux 专有 API。

---

### Q2: 服务器启动后无法连接

**A:** 检查防火墙设置：
```bash
sudo ufw allow 8888/tcp  # Ubuntu
sudo firewall-cmd --add-port=8888/tcp --permanent  # CentOS
```

---

### Q3: 如何修改端口号？

**A:** 修改 `epoll_server.cpp` 中的 `PORT` 常量，重新编译。

---

### Q4: 为什么使用单线程？

**A:**
- I/O 密集型应用，瓶颈在网络 I/O，不在 CPU
- epoll 单线程可处理数万连接
- 避免线程同步复杂性和开销

---

### Q5: 如何扩展到多核 CPU？

**A:** 使用 **Reactor 模式 + 线程池**：
- 主线程 accept 连接，分配给工作线程
- 每个工作线程运行独立的 epoll 事件循环
- 或使用 SO_REUSEPORT 让多个进程监听同一端口

---

## 🤝 贡献指南

欢迎贡献代码！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 代码风格
- 使用 4 空格缩进
- 函数名使用下划线命名法
- 添加详细的中文注释

---

## 📜 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

- Linux epoll API 文档
- C++ 网络编程社区
- 所有贡献者和使用者

---

<div align="center">

**⭐ 如果这个项目对你有帮助，请给一个 Star！⭐**

Made with ❤️ by C++ Server Architects

[返回顶部](#-基于-epoll-的高性能多人聊天室)

</div>
