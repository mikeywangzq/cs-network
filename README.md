# 🌐 CS Network - 计算机网络编程项目集

> 从零开始学习计算机网络协议与网络编程

---

## 📚 项目简介

这是一个计算机网络学习项目集合，通过动手实现各种网络协议和工具，深入理解网络原理。

## 🗂️ 项目列表

### 1. [ICMP Ping 工具](./PING_ICMP/)

**原始套接字实现的网络诊断工具**

使用C++和原始套接字(SOCK_RAW)从零实现一个完整的ping工具。

**核心技术：**
- 原始套接字 (Raw Socket)
- ICMP协议 (Echo Request/Reply)
- Internet校验和算法
- IP数据包解析
- 网络诊断与测试

**快速开始：**
```bash
cd PING_ICMP
make
sudo ./my_ping google.com
```

[查看详细文档 →](./PING_ICMP/README.md)

---

## 🎯 学习路径

推荐的学习顺序：

1. **PING_ICMP** - 掌握ICMP协议和原始套接字基础
2. （更多项目即将添加...）

---

## 🛠️ 环境要求

- **操作系统**: Linux (推荐Ubuntu 20.04+)
- **编译器**: g++ (支持C++11或更高)
- **权限**: 部分项目需要root权限

---

## 📖 参考资料

### 推荐书籍
- 《TCP/IP详解 卷1：协议》- W. Richard Stevens
- 《Unix网络编程 卷1：套接字联网API》- W. Richard Stevens
- 《计算机网络：自顶向下方法》- James F. Kurose

### 在线资源
- [RFC文档](https://www.rfc-editor.org/)
- [Linux Man Pages](https://man7.org/linux/man-pages/)
- [Wireshark文档](https://www.wireshark.org/docs/)

---

## 🤝 贡献

欢迎提交Issue和Pull Request！

---

## 📝 许可证

本项目仅供学习和教育目的使用。

---

**Happy Learning! 🚀**