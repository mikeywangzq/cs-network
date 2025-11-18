# 🌐 CS Network - 网络协议学习项目

<div align="center">

**从零开始学习网络协议和网络编程**

[![Language](https://img.shields.io/badge/Language-C%2B%2B-blue.svg)](https://isocpp.org/)
[![License](https://img.shields.io/badge/License-Educational-green.svg)](LICENSE)

</div>

---

## 📖 项目简介

这是一个网络协议学习项目集合，通过使用 C++ 和底层 Socket API 从零实现各种网络协议，帮助你深入理解网络编程的核心概念。

每个子项目都包含：
- 📝 详细的中文注释
- 📚 完整的协议流程说明
- 🎯 可运行的示例代码
- 📖 学习资源和参考文档

---

## 📂 项目列表

### 📧 SMTP 客户端

**目录**: [`SMTP/`](SMTP/)

一个使用底层 TCP Socket 实现的极简 SMTP（Simple Mail Transfer Protocol）客户端，用于发送电子邮件。

**特性**:
- ✅ 使用 POSIX Socket API 实现 TCP 通信
- ✅ 完整的 SMTP 协议交互流程
- ✅ 支持 AUTH LOGIN 认证（Base64 编码）
- ✅ DNS 解析（gethostbyname）
- ✅ 跨平台支持（Linux/Unix/Windows）

**学习重点**:
- TCP Socket 网络编程
- 应用层协议的"一问一答"通信模式
- Base64 编码算法
- SMTP 协议工作原理

**快速开始**:
```bash
cd SMTP
make
./smtp_client
```

📖 [查看详细文档](SMTP/README.md)

---

## 🚀 即将添加

- 🌐 **HTTP 客户端/服务器** - 实现简单的 HTTP/1.1 协议
- 📡 **DNS 解析器** - 手动构造和解析 DNS 查询
- 🔐 **TCP/UDP Echo 服务器** - 理解传输层协议
- 🌍 **简易代理服务器** - 学习网络流量转发
- 📨 **POP3 客户端** - 接收邮件协议

---

## 🛠️ 开发环境

### 推荐工具

- **编译器**: GCC/Clang (支持 C++11 及以上) 或 MSVC
- **操作系统**: Linux/Unix/macOS/Windows
- **调试工具**:
  - Wireshark - 网络协议分析
  - tcpdump - 命令行抓包工具
  - netcat (nc) - 网络调试工具

### 环境准备

**Ubuntu/Debian:**
```bash
sudo apt-get update
sudo apt-get install build-essential
```

**macOS:**
```bash
xcode-select --install
```

**Windows:**
- 安装 [MinGW-w64](https://www.mingw-w64.org/) 或 [Visual Studio](https://visualstudio.microsoft.com/)

---

## 📚 学习资源

### 推荐书籍

- 📖 《TCP/IP 详解 卷1：协议》- W. Richard Stevens
- 📖 《Unix 网络编程》- W. Richard Stevens
- 📖 《计算机网络：自顶向下方法》- James F. Kurose

### 在线资源

- [RFC 文档](https://www.rfc-editor.org/) - 网络协议标准
- [Beej's Guide to Network Programming](https://beej-zhcn.netdpi.net/) - Socket 编程指南
- [Wireshark 教程](https://www.wireshark.org/docs/) - 网络协议分析

### 工具推荐

- [MailHog](https://github.com/mailhog/MailHog) - SMTP 测试服务器
- [Postman](https://www.postman.com/) - API 测试工具
- [HTTPie](https://httpie.io/) - 命令行 HTTP 客户端

---

## 🎯 学习路径建议

1. **基础阶段**
   - 学习 TCP/IP 基本概念
   - 掌握 Socket 编程基础
   - 理解客户端-服务器模型

2. **协议实现**
   - 从简单的文本协议开始（如 SMTP、POP3）
   - 逐步学习二进制协议（如 DNS）
   - 了解 HTTP 等复杂协议

3. **进阶学习**
   - 添加加密支持（TLS/SSL）
   - 实现异步 I/O
   - 性能优化

---

## 🤝 贡献

欢迎提出改进建议和问题！

---

## ⚠️ 注意事项

- 本项目仅用于**学习和教育目的**
- 部分实现为了简化学习，**未包含生产环境所需的安全特性**
- 请勿在生产环境中直接使用这些代码
- 使用网络工具时请遵守相关法律法规

---

## 📄 许可

本项目采用教育许可，仅供学习使用。

---

<div align="center">

**🌐 Happy Learning!**

如果这个项目对你有帮助，欢迎 ⭐ Star！

</div>