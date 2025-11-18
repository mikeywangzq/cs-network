# 🌐 CS Network - 计算机网络实验项目集合

[![Language](https://img.shields.io/badge/Language-C++-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux-green.svg)](https://www.linux.org/)
[![License](https://img.shields.io/badge/License-Educational-yellow.svg)](LICENSE)

一个用于学习计算机网络底层原理和协议实现的实验项目集合。通过手写网络工具，深入理解 TCP/IP 协议栈、套接字编程和网络诊断技术。

## 📚 项目目录

### 🔍 [Traceroute - 路径跟踪工具](./Traceroute)

一个用 C++ 编写的极简 Traceroute 实现，用于追踪数据包到目标主机所经过的路由器路径。

**核心技术**:
- ✅ TTL (Time-To-Live) 机制
- ✅ ICMP (互联网控制消息协议)
- ✅ Raw Socket (原始套接字编程)
- ✅ UDP 探测包发送
- ✅ 网络诊断技术

**快速开始**:
```bash
cd Traceroute
make
sudo ./my_traceroute google.com
```

**详细文档**: [Traceroute README](./Traceroute/README.md)

---

## 🎯 项目目标

通过实现各种网络工具和协议，掌握以下核心知识：

- 📡 **套接字编程**: 理解 BSD Socket API 的使用
- 🔧 **协议实现**: 深入学习 IP、ICMP、TCP、UDP 等协议
- 🛡️ **网络诊断**: 掌握网络故障排查和性能分析技术
- 🌍 **底层原理**: 了解数据包在网络中的传输过程
- 💻 **系统编程**: 熟悉 Linux 系统调用和原始套接字

## 🛠️ 技术栈

| 技术 | 说明 |
|------|------|
| **编程语言** | C++ (C++11 及以上) |
| **操作系统** | Linux (支持 POSIX) |
| **网络协议** | TCP/IP, ICMP, UDP, DNS |
| **编程接口** | BSD Sockets, Raw Sockets |
| **工具链** | g++, make, gdb |

## 📋 系统要求

- **操作系统**: Linux (Ubuntu 18.04+ 推荐)
- **编译器**: g++ 7.0 或更高版本
- **权限**: 某些工具需要 root 权限 (sudo)
- **网络**: 能够访问互联网

## 🚀 快速开始

### 克隆仓库

```bash
git clone <repository-url>
cd cs-network
```

### 选择项目

每个子目录都是一个独立的项目，包含完整的源代码、Makefile 和说明文档。

```bash
# 进入具体项目目录
cd Traceroute

# 查看项目说明
cat README.md

# 编译项目
make

# 运行项目（部分需要 sudo）
sudo ./my_traceroute google.com
```

## 📖 学习路径

### 初级 - 基础套接字编程
- 🔍 **Traceroute**: 学习 Raw Socket、ICMP、TTL 机制

### 中级 - 协议实现（规划中）
- 📡 **Ping**: ICMP Echo Request/Reply
- 🌐 **简易 DNS 客户端**: DNS 查询和解析
- 🔌 **TCP Echo Server/Client**: TCP 连接管理

### 高级 - 复杂网络应用（规划中）
- 🔥 **简易防火墙**: 包过滤和规则匹配
- 📊 **网络嗅探器**: 数据包捕获和分析
- 🚄 **HTTP 服务器**: 应用层协议实现

## 🎓 核心概念

### 1. TTL (Time-To-Live)
- 防止数据包在网络中无限循环
- 每经过一个路由器减 1
- 减到 0 时路由器返回 ICMP Time Exceeded

### 2. ICMP (Internet Control Message Protocol)
- 用于网络诊断和错误报告
- 常见类型：Echo Request/Reply (8/0)、Time Exceeded (11)、Destination Unreachable (3)
- Ping 和 Traceroute 的基础协议

### 3. Raw Socket (原始套接字)
- 允许直接访问底层协议
- 可以自定义协议头部
- 通常需要 root 权限
- 用于实现网络工具和协议分析器

### 4. 套接字选项 (Socket Options)
- `IP_TTL`: 设置 IP 包的生存时间
- `SO_RCVTIMEO`: 设置接收超时
- `SO_SNDTIMEO`: 设置发送超时
- `SO_REUSEADDR`: 允许重用本地地址

## 🔬 调试技巧

### 使用 tcpdump 查看网络包

```bash
# 捕获 ICMP 包
sudo tcpdump -i any icmp -nn

# 捕获特定主机的包
sudo tcpdump -i any host google.com -nn

# 显示包内容（十六进制）
sudo tcpdump -i any icmp -XX
```

### 使用 Wireshark 分析

```bash
# 启动 Wireshark（图形界面）
sudo wireshark
```

### 使用 strace 跟踪系统调用

```bash
# 跟踪程序的系统调用
sudo strace ./my_traceroute google.com
```

## 📚 推荐资源

### 书籍
- 📖 《TCP/IP 详解 卷1：协议》- W. Richard Stevens
- 📖 《Unix 网络编程 卷1：套接字联网 API》- W. Richard Stevens
- 📖 《计算机网络：自顶向下方法》- James F. Kurose

### RFC 文档
- [RFC 791 - Internet Protocol (IP)](https://tools.ietf.org/html/rfc791)
- [RFC 792 - Internet Control Message Protocol (ICMP)](https://tools.ietf.org/html/rfc792)
- [RFC 768 - User Datagram Protocol (UDP)](https://tools.ietf.org/html/rfc768)
- [RFC 793 - Transmission Control Protocol (TCP)](https://tools.ietf.org/html/rfc793)

### 在线资源
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Linux Socket Programming](https://www.kernel.org/doc/html/latest/networking/index.html)

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 贡献内容可以包括：
- 🐛 Bug 修复
- ✨ 新功能实现
- 📝 文档改进
- 🎨 代码优化
- 💡 新项目建议

## ⚠️ 注意事项

1. **权限要求**: 许多网络工具需要 root 权限才能创建原始套接字
2. **安全性**: 这些工具仅用于学习目的，请勿用于未经授权的网络扫描
3. **防火墙**: 某些网络环境可能会拦截 ICMP 或其他协议的数据包
4. **责任**: 使用这些工具时请遵守网络使用规范和法律法规

## 📝 许可证

本项目仅用于教育和学习目的。

## 📬 联系方式

如有问题或建议，请提交 Issue 或 Pull Request。

---

<div align="center">

**🌟 如果这个项目对你有帮助，请给个 Star！🌟**

Made with ❤️ for Network Programming Learners

</div>
