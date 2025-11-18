# 🌐 CS Network Programming Projects

这是一个网络编程学习项目集合，包含多个基于 C/C++ 的网络应用实现。

## 📚 项目列表

### 1. 📁 简易 FTP 文件传输系统 (`/FTP`)

一个完整的 C/S 架构文件传输工具，使用底层 TCP Socket API 实现。

**主要特性：**
- ✅ 基于 POSIX Socket API 的底层网络编程
- ✅ 支持多客户端并发连接（多线程）
- ✅ 文件上传、下载、列表功能
- ✅ 自定义应用层协议
- ✅ 健壮的错误处理和数据传输
- ✅ 详细的中文代码注释

**快速开始：**
```bash
cd FTP
make              # 编译
./server          # 启动服务器（终端1）
./client          # 启动客户端（终端2）
```

详细文档请查看：[FTP/README.md](FTP/README.md)

---

## 🎯 学习目标

通过这些项目，你将学习到：

- **Socket 编程基础**：创建、绑定、监听、连接等核心操作
- **TCP 协议**：可靠传输、流控制、连接管理
- **并发处理**：多线程编程、线程同步
- **协议设计**：自定义应用层协议
- **文件 I/O**：高效的文件读写操作
- **错误处理**：网络异常、边界条件的处理
- **跨平台开发**：Linux/Unix/macOS 兼容性

---

## 🛠️ 开发环境

- **编译器**：g++ (支持 C++11 或更高版本)
- **平台**：Linux / macOS / Unix-like 系统
- **依赖**：无外部依赖，仅使用标准库和 POSIX API

---

## 📖 参考资料

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [TCP/IP 详解 卷1：协议](https://book.douban.com/subject/1088054/)
- [Unix Network Programming](https://book.douban.com/subject/1500149/)

---

## 📄 许可证

MIT License

---

**Happy Coding! 🚀**