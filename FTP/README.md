# 📁 简易 FTP 文件传输系统

[![C++](https://img.shields.io/badge/Language-C%2B%2B11-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20macOS-lightgrey.svg)](https://www.linux.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

一个基于 C++ 和底层 TCP Socket API 实现的简单 C/S 架构文件传输工具，用于学习网络编程和理解 FTP 协议的基本原理。

---

## ✨ 特性

- 🔌 **底层 Socket 编程**：使用 POSIX Socket API，不依赖高级网络库
- 🧵 **多客户端支持**：服务器支持多线程并发处理多个客户端连接
- 📤 **文件上传**：客户端可以上传本地文件到服务器
- 📥 **文件下载**：客户端可以从服务器下载文件
- 📋 **文件列表**：查看服务器上的所有可用文件
- 🔒 **安全性考虑**：防止路径遍历攻击，文件名合法性检查
- 📊 **进度显示**：大文件传输时显示实时进度
- 💪 **健壮的网络传输**：正确处理 send/recv 返回值，确保数据完整性
- 🌐 **网络字节序转换**：正确处理跨平台字节序问题

---

## 📋 目录结构

```
FTP/
├── server.cpp          # 服务器端源代码
├── client.cpp          # 客户端源代码
├── Makefile            # 编译脚本
├── README.md           # 本文档
├── server_files/       # 服务器文件存储目录
│   ├── test.txt        # 示例文件
│   └── hello.txt       # 示例文件
└── client_files/       # 客户端下载文件存储目录
```

---

## 🚀 快速开始

### 1. 编译程序

```bash
cd FTP
make
```

或者分别编译服务器和客户端：

```bash
make server    # 只编译服务器
make client    # 只编译客户端
```

### 2. 启动服务器

在一个终端窗口中运行：

```bash
./server
```

服务器将在端口 **9999** 上监听客户端连接。

### 3. 启动客户端

在另一个终端窗口中运行：

```bash
./client
```

客户端将自动连接到本地服务器（127.0.0.1:9999）。

---

## 🎮 使用指南

### 客户端命令

连接成功后，您可以使用以下命令：

| 命令 | 说明 | 示例 |
|------|------|------|
| `ls` | 列出服务器上的所有文件 | `ls` |
| `upload <filename>` | 上传文件到服务器 | `upload test.txt` |
| `download <filename>` | 从服务器下载文件 | `download hello.txt` |
| `help` | 显示帮助信息 | `help` |
| `quit` | 退出客户端 | `quit` |

### 使用示例

```bash
ftp> ls
========== 服务器文件列表 ==========
  test.txt
  hello.txt
====================================

ftp> upload myfile.txt
[INFO] 准备上传文件: myfile.txt (大小: 1024 字节)
[SERVER] OK
[INFO] 开始上传文件...
[PROGRESS] 已上传: 1024/1024 (100%)
[SUCCESS] 文件上传成功！

ftp> download test.txt
[INFO] 准备下载文件: test.txt
[SERVER] OK
[INFO] 文件大小: 512 字节
[INFO] 开始下载文件...
[PROGRESS] 已下载: 512/512 (100%)
[SUCCESS] 文件下载成功，保存至: client_files/test.txt

ftp> quit
[INFO] 发送退出命令...
[INFO] 已断开连接，程序退出。
```

---

## 🔧 自定义应用层协议

本项目实现了一个简单的文本命令 + 二进制数据的协议：

### 1️⃣ LIST（列出文件）

**客户端 → 服务器：**
```
LIST\n
```

**服务器 → 客户端：**
```
file1.txt\n
file2.txt\n
...
END_OF_LIST\n
```

### 2️⃣ UPLOAD（上传文件）

**客户端 → 服务器：**
```
UPLOAD <filename>\n
```

**服务器 → 客户端：**
```
OK\n          # 或 ERROR file exists\n
```

**如果返回 OK，客户端继续发送：**
1. 文件大小（8字节，uint64_t，网络字节序）
2. 文件内容（二进制数据流）

### 3️⃣ DOWNLOAD（下载文件）

**客户端 → 服务器：**
```
DOWNLOAD <filename>\n
```

**服务器 → 客户端：**
```
OK\n          # 或 ERROR not found\n
```

**如果返回 OK，服务器继续发送：**
1. 文件大小（8字节，uint64_t，网络字节序）
2. 文件内容（二进制数据流）

### 4️⃣ QUIT（退出）

**客户端 → 服务器：**
```
QUIT\n
```

服务器和客户端都关闭连接。

---

## 🛡️ 安全特性

1. **文件名验证**：拒绝包含 `..`、`/`、`\` 的文件名，防止路径遍历攻击
2. **文件存在性检查**：上传前检查文件是否已存在，下载前检查文件是否存在
3. **健壮的数据传输**：
   - `send_all()` 和 `recv_all()` 函数确保数据完整传输
   - 正确处理部分发送/接收的情况
4. **网络字节序转换**：使用 `htonll()` 和 `ntohll()` 确保跨平台兼容性

---

## 🏗️ 技术要点

### 服务器端 (server.cpp)

- **Socket 创建与绑定**：使用 `socket()`, `bind()`, `listen()`, `accept()`
- **多线程并发**：为每个客户端创建独立线程（`std::thread`）
- **目录遍历**：使用 `opendir()` 和 `readdir()` 列出文件
- **文件 I/O**：使用 C++ `fstream` 进行文件读写

### 客户端 (client.cpp)

- **Socket 连接**：使用 `socket()` 和 `connect()` 连接服务器
- **命令行交互**：`getline()` 读取用户输入，`stringstream` 解析命令
- **文件传输**：使用缓冲区（4KB）分块传输，减少内存占用

### 关键函数

```cpp
// 健壮的发送函数 - 确保所有数据都被发送
bool send_all(SOCKET sock, const char* buffer, size_t length);

// 健壮的接收函数 - 确保接收指定长度的数据
bool recv_all(SOCKET sock, char* buffer, size_t length);

// 网络字节序转换（64位）
uint64_t htonll(uint64_t value);  // 主机序 → 网络序
uint64_t ntohll(uint64_t value);  // 网络序 → 主机序
```

---

## 📚 编译选项

### Makefile 命令

| 命令 | 说明 |
|------|------|
| `make` | 编译服务器和客户端 |
| `make server` | 只编译服务器 |
| `make client` | 只编译客户端 |
| `make clean` | 清理编译产物 |
| `make setup` | 创建必要的目录 |
| `make run-server` | 编译并运行服务器 |
| `make run-client` | 编译并运行客户端 |
| `make help` | 显示帮助信息 |

### 编译器要求

- **C++ 标准**：C++11 或更高版本
- **编译器**：g++, clang++ 或其他支持 C++11 的编译器
- **平台**：Linux, macOS（或其他 POSIX 兼容系统）

---

## 🔍 测试建议

### 1. 基本功能测试

```bash
# 终端 1：启动服务器
./server

# 终端 2：启动客户端
./client
ftp> ls                      # 查看服务器文件
ftp> download test.txt       # 下载文件
ftp> upload myfile.txt       # 上传文件
ftp> quit                    # 退出
```

### 2. 多客户端测试

打开多个终端，同时运行多个客户端，测试服务器的并发处理能力。

### 3. 大文件测试

```bash
# 创建一个 100MB 的测试文件
dd if=/dev/zero of=large_file.bin bs=1M count=100

# 上传大文件，观察进度显示
ftp> upload large_file.bin
```

### 4. 错误处理测试

- 尝试下载不存在的文件
- 尝试上传已存在的文件
- 测试非法文件名（包含 `../` 等）

---

## ⚠️ 注意事项

1. **本项目仅用于学习目的**，不应用于生产环境
2. **未实现身份认证**，任何人都可以连接和传输文件
3. **未加密传输**，数据以明文传输（包括文件内容）
4. **端口固定**：默认使用 9999 端口，可在代码中修改
5. **服务器 IP**：客户端默认连接 127.0.0.1（本地），远程连接需修改代码

---

## 🛠️ 常见问题

### Q1: 编译时出现 "undefined reference to pthread" 错误

**A:** 确保 Makefile 中包含 `-pthread` 选项：
```makefile
CXXFLAGS = -std=c++11 -Wall -pthread
```

### Q2: 服务器启动失败，提示 "bind 失败"

**A:** 可能端口被占用，尝试：
```bash
# 查看端口占用
lsof -i :9999
# 或修改 server.cpp 中的 PORT 常量
```

### Q3: 客户端无法连接服务器

**A:** 确保：
1. 服务器已启动并正常运行
2. 防火墙允许 9999 端口通信
3. 客户端和服务器在同一网络（或修改 SERVER_IP）

### Q4: 文件传输中断或不完整

**A:** 这通常是网络问题，程序已实现健壮的传输机制。如果问题持续，检查：
1. 网络连接是否稳定
2. 磁盘空间是否充足
3. 文件权限是否正确

---

## 📖 学习资源

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/) - 经典的 Socket 编程教程
- [TCP/IP 详解 卷1：协议](https://book.douban.com/subject/1088054/)
- [POSIX Socket API 文档](https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/sys_socket.h.html)

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进这个项目！

---

## 📄 许可证

本项目采用 MIT 许可证 - 详见 [LICENSE](LICENSE) 文件

---

## 👨‍💻 作者

网络编程学习项目

---

## 🎯 后续改进方向

- [ ] 添加用户认证机制（用户名/密码）
- [ ] 实现 SSL/TLS 加密传输
- [ ] 支持断点续传
- [ ] 添加文件完整性校验（MD5/SHA256）
- [ ] 实现文件删除、重命名等功能
- [ ] 添加传输速度限制（限速）
- [ ] 支持 Windows 平台（Winsock）
- [ ] 图形界面客户端（GUI）
- [ ] 日志记录系统
- [ ] 配置文件支持

---

**Happy Coding! 🚀**
