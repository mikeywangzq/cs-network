<div align="center">

# 🌐 CS Network - 网络协议学习项目

**从零开始深入学习网络协议和网络编程**

[![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![Standard](https://img.shields.io/badge/C%2B%2B-11-brightgreen.svg)](https://en.wikipedia.org/wiki/C%2B%2B11)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS%20%7C%20Windows-lightgrey.svg)](https://github.com)
[![License](https://img.shields.io/badge/License-Educational-green.svg)](LICENSE)

*通过手写底层实现，深入理解网络协议的精髓*

[English](README_EN.md) | 简体中文

</div>

---

## 📖 项目简介

这是一个**完全从零开始**的网络协议学习项目集合，通过使用 C++ 和底层 Socket API 手动实现各种网络协议，帮助你真正理解网络编程的核心原理。

### 🎯 项目特色

- 🔧 **完全手动实现** - 不使用高级库，直接操作 Socket API
- 📦 **协议层次分明** - 从数据链路层到应用层的完整实现
- 📚 **详细中文注释** - 每个关键步骤都有清晰的说明
- 🎓 **教育价值高** - 深入理解二进制协议、网络字节序、状态机等核心概念
- 💻 **跨平台支持** - Linux、macOS、Windows 全平台支持
- 🚀 **可运行示例** - 每个项目都包含完整的示例代码

### 📦 你将学到

- ✅ TCP/UDP Socket 编程基础
- ✅ 应用层协议（SMTP、POP3、DNS、HTTP）
- ✅ 传输层协议（TCP 状态机、连接跟踪）
- ✅ I/O 多路复用（epoll 高性能服务器）
- ✅ 网络抓包与协议分析
- ✅ 二进制协议构造与解析
- ✅ 网络字节序转换
- ✅ Base64 编码算法

---

## 🗂️ 项目列表

<table>
<thead>
<tr>
<th width="15%">项目</th>
<th width="45%">描述</th>
<th width="20%">核心技术</th>
<th width="20%">难度</th>
</tr>
</thead>
<tbody>

<tr>
<td><strong>📧 SMTP 客户端</strong><br><a href="SMTP/">查看详情</a></td>
<td>
使用底层 TCP Socket 实现的邮件发送客户端，支持 AUTH LOGIN 认证和 Base64 编码。
<br><br>
<strong>学习重点：</strong>
<ul>
<li>TCP 连接建立</li>
<li>应用层文本协议</li>
<li>Base64 编码实现</li>
<li>DNS 解析</li>
</ul>
</td>
<td>
• TCP Socket<br>
• SMTP 协议<br>
• Base64 编码<br>
• 跨平台编程
</td>
<td>⭐⭐ 入门</td>
</tr>

<tr>
<td><strong>📬 POP3 客户端</strong><br><a href="POP3/">查看详情</a></td>
<td>
从零实现的邮件接收客户端，完整演示 POP3 协议的三状态模型（认证、事务、更新）。
<br><br>
<strong>学习重点：</strong>
<ul>
<li>协议状态机</li>
<li>多行响应解析</li>
<li>邮件内容提取</li>
<li>错误处理机制</li>
</ul>
</td>
<td>
• POP3 协议<br>
• 状态机设计<br>
• 多行数据处理<br>
• 授权码认证
</td>
<td>⭐⭐ 入门</td>
</tr>

<tr>
<td><strong>🌐 DNS 解析器</strong><br><a href="nslookup/">查看详情</a></td>
<td>
类似 <code>nslookup</code> 的 DNS 查询工具，手动构建 DNS 查询包，解析响应（支持指针压缩）。
<br><br>
<strong>学习重点：</strong>
<ul>
<li>二进制协议构造</li>
<li>DNS 消息压缩</li>
<li>UDP Socket 编程</li>
<li>网络字节序转换</li>
</ul>
</td>
<td>
• UDP Socket<br>
• DNS 协议 (RFC 1035)<br>
• 二进制数据包<br>
• 指针压缩解析
</td>
<td>⭐⭐⭐ 中级</td>
</tr>

<tr>
<td><strong>📡 TCP 协议分析器</strong><br><a href="TCP_Analyzer/">查看详情</a></td>
<td>
基于 Linux AF_PACKET 原始套接字的有状态 TCP 连接跟踪器，实现完整的 TCP 状态机（三次握手、四次挥手）。
<br><br>
<strong>学习重点：</strong>
<ul>
<li>原始套接字捕包</li>
<li>TCP 状态机实现</li>
<li>连接规范化技术</li>
<li>协议层次解析</li>
</ul>
</td>
<td>
• AF_PACKET<br>
• TCP 状态机<br>
• 连接跟踪<br>
• 以太网/IP/TCP 解析
</td>
<td>⭐⭐⭐⭐⭐ 高级</td>
</tr>

<tr>
<td><strong>💬 Epoll 聊天室</strong><br><a href="cs-chatroom/">查看详情</a></td>
<td>
基于 Linux epoll 的高性能多人聊天室服务器，单线程管理 1000+ 并发连接，采用边缘触发模式。
<br><br>
<strong>学习重点：</strong>
<ul>
<li>I/O 多路复用</li>
<li>非阻塞 Socket</li>
<li>事件驱动架构</li>
<li>高并发设计</li>
</ul>
</td>
<td>
• epoll API<br>
• 非阻塞 I/O<br>
• 边缘触发 (ET)<br>
• 单线程事件循环
</td>
<td>⭐⭐⭐⭐ 高级</td>
</tr>

</tbody>
</table>

---

## 🚀 快速开始

### 环境要求

- **编译器**: GCC/Clang (C++11+) 或 MSVC
- **操作系统**: Linux/Unix/macOS/Windows
- **工具**: make, git

### 克隆仓库

```bash
git clone https://github.com/mikeywangzq/cs-network.git
cd cs-network
```

### 运行示例项目

```bash
# 1. SMTP 客户端（发送邮件）
cd SMTP
make
./smtp_client

# 2. POP3 客户端（接收邮件）
cd ../POP3
g++ -o pop3_client pop3_client.cpp
./pop3_client

# 3. DNS 解析器（域名查询）
cd ../nslookup
make
./resolver google.com

# 4. TCP 协议分析器（需要 sudo）
cd ../TCP_Analyzer
make
sudo ./tcp_analyzer lo

# 5. Epoll 聊天室服务器
cd ../cs-chatroom
make
./epoll_server
# 在另一个终端运行客户端
./client
```

---

## 📚 学习路径建议

### 🌱 初级路线（适合新手）

```
1️⃣ SMTP 客户端 → 学习 TCP Socket 基础
2️⃣ POP3 客户端 → 理解协议状态机
3️⃣ DNS 解析器 → 掌握 UDP 和二进制协议
```

**建议学习时间**: 2-3 周
**前置知识**: C++ 基础、计算机网络基础

### 🚀 高级路线（适合进阶）

```
4️⃣ TCP 协议分析器 → 深入理解 TCP 状态机和原始套接字
5️⃣ Epoll 聊天室 → 掌握高性能服务器设计
```

**建议学习时间**: 3-4 周
**前置知识**: 完成初级路线、熟悉 Linux 系统编程

---

## 🛠️ 开发环境配置

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install build-essential g++ make git wireshark
```

### macOS

```bash
# 安装 Xcode Command Line Tools
xcode-select --install

# 安装 Wireshark（可选）
brew install wireshark
```

### Windows

**方式 1: MinGW-w64**
```bash
# 下载并安装 MinGW-w64
# https://www.mingw-w64.org/downloads/
```

**方式 2: Visual Studio**
```bash
# 安装 Visual Studio Community
# https://visualstudio.microsoft.com/
```

---

## 📊 技术栈对比

| 技术 | SMTP | POP3 | DNS | TCP Analyzer | Epoll 聊天室 |
|------|------|------|-----|--------------|--------------|
| **Socket 类型** | TCP | TCP | UDP | RAW | TCP |
| **协议层** | 应用层 | 应用层 | 应用层 | 传输层 | 应用层 |
| **I/O 模型** | 阻塞 | 阻塞 | 阻塞 | 阻塞 | 非阻塞 + epoll |
| **状态管理** | 无 | 有 | 无 | 有（复杂） | 有 |
| **平台支持** | 全平台 | 全平台 | 全平台 | 仅 Linux | 仅 Linux |
| **并发模型** | 单连接 | 单连接 | 单请求 | 被动捕获 | 多连接（1000+） |

---

## 🔬 核心技术详解

### 🔹 协议实现技术

- **TCP Socket 编程**: 连接建立、数据收发、错误处理
- **UDP Socket 编程**: 无连接数据报、超时处理
- **原始套接字 (RAW Socket)**: 直接操作以太网帧
- **AF_PACKET**: Linux 数据链路层捕包

### 🔹 协议解析技术

- **文本协议解析**: SMTP/POP3 命令响应解析
- **二进制协议构造**: DNS 查询包手动构建
- **协议层次解析**: Ethernet → IP → TCP 分层解析
- **DNS 消息压缩**: 指针跳转（0xC0XX）

### 🔹 高级技术

- **I/O 多路复用**: epoll 边缘触发 (ET) 模式
- **状态机设计**: POP3 三状态、TCP 11 状态
- **连接规范化**: 双向连接识别算法
- **非阻塞 I/O**: fcntl 设置 O_NONBLOCK

---

## 📖 学习资源

### 📕 推荐书籍

| 书名 | 作者 | 适合阶段 |
|------|------|----------|
| 《TCP/IP 详解 卷1：协议》 | W. Richard Stevens | 必读经典 |
| 《Unix 网络编程 卷1：套接字联网 API》 | W. Richard Stevens | Socket 编程圣经 |
| 《计算机网络：自顶向下方法》 | James F. Kurose | 理论基础 |
| 《Linux 高性能服务器编程》 | 游双 | 进阶实战 |

### 🌐 在线资源

- [RFC 文档](https://www.rfc-editor.org/) - 网络协议官方标准
- [Beej's Guide to Network Programming（中文版）](https://beej-zhcn.netdpi.net/) - Socket 编程入门
- [Linux Man Pages](https://man7.org/linux/man-pages/) - Linux 系统调用文档
- [Wireshark 教程](https://www.wireshark.org/docs/) - 网络抓包分析

### 🛠️ 推荐工具

| 工具 | 用途 | 链接 |
|------|------|------|
| **Wireshark** | 网络协议分析 | [官网](https://www.wireshark.org/) |
| **tcpdump** | 命令行抓包 | `apt install tcpdump` |
| **netcat (nc)** | 网络调试工具 | `apt install netcat` |
| **MailHog** | SMTP 测试服务器 | [GitHub](https://github.com/mailhog/MailHog) |
| **curl** | HTTP 客户端 | `apt install curl` |
| **telnet** | 协议调试 | `apt install telnet` |

---

## ⚠️ 注意事项

### 安全警告

- 🔴 **本项目仅用于学习和教育目的**
- 🔴 **未实现 SSL/TLS 加密，不要用于生产环境**
- 🔴 **不要在代码中硬编码真实密码**
- 🔴 **使用网络工具时请遵守相关法律法规**

### 限制说明

- ❌ 部分项目未包含生产环境所需的安全特性
- ❌ 简化了错误处理和异常情况
- ❌ 未实现完整的 RFC 标准（仅核心功能）
- ❌ 部分项目仅支持 Linux 平台

### 适用场景

- ✅ 学习网络协议原理
- ✅ 理解底层 Socket 编程
- ✅ 本地测试和实验
- ✅ 面试准备和技术提升

---

## 🤝 贡献指南

欢迎提出改进建议和问题！

### 贡献方式

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送到分支 (`git push origin feature/AmazingFeature`)
5. 开启 Pull Request

### 改进建议

- 📝 添加更多协议实现（HTTP、FTP、Telnet）
- 🔐 添加 SSL/TLS 支持
- 🧪 添加单元测试
- 📚 改进文档和示例
- 🌍 添加英文文档

---

## 🎯 进阶扩展方向

完成基础项目后，可以尝试以下扩展：

### 🔸 协议扩展

- [ ] **HTTP/1.1 服务器** - 实现完整的 HTTP 协议
- [ ] **WebSocket 服务器** - 实时双向通信
- [ ] **FTP 客户端** - 文件传输协议
- [ ] **SSH 客户端** - 加密远程登录（需要加密库）

### 🔸 性能优化

- [ ] **多线程/多进程模型** - 提升并发能力
- [ ] **内存池管理** - 减少内存分配开销
- [ ] **零拷贝技术** - sendfile、splice
- [ ] **SO_REUSEPORT** - 多进程负载均衡

### 🔸 安全增强

- [ ] **TLS/SSL 支持** - 使用 OpenSSL
- [ ] **DNSSEC 验证** - DNS 安全扩展
- [ ] **SYN Flood 防护** - DDoS 防护
- [ ] **OAuth 2.0 认证** - 现代认证机制

---

## 📊 项目统计

- **总代码量**: ~3000+ 行（含注释）
- **支持协议**: 5+ 种网络协议
- **学习周期**: 4-8 周
- **难度分布**: 入门 40% | 中级 20% | 高级 40%

---

## 📄 许可证

本项目采用教育许可，仅供学习使用。详见 [LICENSE](LICENSE) 文件。

---

## 🙏 致谢

感谢所有为网络协议标准化做出贡献的组织和个人：

- IETF (Internet Engineering Task Force)
- RFC 编辑委员会
- Linux 内核开发团队
- C++ 开源社区
- 所有贡献者和学习者

---

<div align="center">

## 🌟 Star History

[![Star History Chart](https://api.star-history.com/svg?repos=mikeywangzq/cs-network&type=Date)](https://star-history.com/#mikeywangzq/cs-network&Date)

---

**🎓 从零开始，深入本质**

**💪 理论结合实践，掌握网络编程**

**🚀 如果这个项目对你有帮助，欢迎 ⭐ Star！**

Made with ❤️ by Network Protocol Enthusiasts

[⬆ 返回顶部](#-cs-network---网络协议学习项目)

</div>