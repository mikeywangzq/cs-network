# CS Network - 计算机网络学习项目集合

这个仓库包含了多个计算机网络相关的学习项目，使用 C++ 和底层网络 API 实现。

## 📁 项目列表

### 🌐 [BitTorrent - 简化版 P2P 文件分享工具](./BitTorrent/)

一个教育性的 P2P 文件分享系统，模仿 BitTorrent 的核心思想。

**核心特性：**
- ✅ Tracker 服务器和 Peer 客户端实现
- ✅ 多线程并发架构
- ✅ 文件分片和位域管理
- ✅ 自定义 P2P 协议（握手、位域交换、片段传输）
- ✅ 多源并行下载

**技术栈：**
- C++11
- POSIX Socket API
- std::thread 多线程
- TCP 协议

**快速开始：**
```bash
cd BitTorrent
make
./test.sh  # 运行自动化测试
```

详细文档请查看 [BitTorrent/README.md](./BitTorrent/README.md)

---

## 🎯 学习目标

这些项目旨在帮助理解：
- Socket 网络编程基础
- TCP/IP 协议栈
- 多线程并发编程
- P2P 网络原理
- 网络协议设计

## 📚 相关资源

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [TCP/IP Illustrated](https://en.wikipedia.org/wiki/TCP/IP_Illustrated)
- [Computer Networking: A Top-Down Approach](https://gaia.cs.umass.edu/kurose_ross/index.php)

## 📄 许可证

本项目仅用于教育目的。