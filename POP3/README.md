# 📬 POP3 客户端 - 极简学习版

> 一个使用 C++ 和底层 TCP Socket API 实现的 POP3 邮件客户端，用于学习网络编程和邮件协议。

## 📋 目录

- [项目简介](#项目简介)
- [功能特性](#功能特性)
- [POP3 协议简介](#pop3-协议简介)
- [编译与运行](#编译与运行)
- [使用说明](#使用说明)
- [代码结构](#代码结构)
- [重要提醒](#重要提醒)
- [常见问题](#常见问题)
- [学习资源](#学习资源)

---

## 🎯 项目简介

本项目是一个**教学性质**的 POP3 客户端实现，使用纯 C++ 和底层 Socket API（POSIX Sockets / Winsock）编写。通过这个项目，你可以学习：

- TCP Socket 编程基础
- POP3 邮件协议的工作原理
- 网络协议的"一问一答"模式
- 跨平台网络编程技巧（Linux/Windows）

## ✨ 功能特性

- ✅ **连接到 POP3 服务器**（明文端口 110）
- ✅ **用户身份验证**（USER/PASS 命令）
- ✅ **获取邮箱状态**（STAT 命令）
- ✅ **列出所有邮件**（LIST 命令）
- ✅ **下载邮件内容**（RETR 命令）
- ✅ **删除邮件**（DELE 命令，代码中已注释）
- ✅ **优雅退出**（QUIT 命令）
- ✅ **跨平台支持**（Linux/macOS/Windows）
- ✅ **详细中文注释**

## 📡 POP3 协议简介

### 什么是 POP3？

**POP3**（Post Office Protocol version 3）是一种用于从邮件服务器接收电子邮件的应用层协议（RFC 1939）。它是一个简单的"一问一答"式的文本协议。

### 协议特点

- **端口**：明文 110，加密（SSL/TLS）995
- **响应格式**：
  - 成功：`+OK <message>`
  - 失败：`-ERR <error message>`
- **命令格式**：每条命令以 `\r\n` 结尾

### POP3 状态机

```
┌─────────────────┐
│  连接建立       │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ AUTHORIZATION   │  ◄── USER/PASS 命令
│  (认证状态)     │
└────────┬────────┘
         │ 认证成功
         ▼
┌─────────────────┐
│ TRANSACTION     │  ◄── STAT/LIST/RETR/DELE 命令
│  (事务状态)     │
└────────┬────────┘
         │ QUIT 命令
         ▼
┌─────────────────┐
│ UPDATE          │  ◄── 执行删除操作，释放锁
│  (更新状态)     │
└─────────────────┘
```

### 主要命令

| 命令 | 格式 | 功能 | 响应 |
|------|------|------|------|
| **USER** | `USER username` | 指定用户名 | `+OK` |
| **PASS** | `PASS password` | 提供密码 | `+OK maildrop locked` |
| **STAT** | `STAT` | 获取邮箱状态 | `+OK <count> <size>` |
| **LIST** | `LIST [msg]` | 列出邮件 | 多行响应，以 `.\r\n` 结束 |
| **RETR** | `RETR <msg>` | 下载邮件 | 多行响应，以 `.\r\n` 结束 |
| **DELE** | `DELE <msg>` | 标记删除 | `+OK` |
| **QUIT** | `QUIT` | 退出并提交更改 | `+OK` |

---

## 🔧 编译与运行

### 前置要求

- **C++ 编译器**：
  - Linux/macOS: `g++` 或 `clang++`
  - Windows: `MSVC` (Visual Studio) 或 `MinGW`

### 编译命令

#### Linux / macOS

```bash
cd POP3
g++ -o pop3_client pop3_client.cpp
```

#### Windows (Visual Studio)

```cmd
cd POP3
cl pop3_client.cpp /link ws2_32.lib
```

#### Windows (MinGW)

```bash
cd POP3
g++ -o pop3_client.exe pop3_client.cpp -lws2_32
```

### 运行

```bash
# Linux/macOS
./pop3_client

# Windows
pop3_client.exe
```

---

## 🚀 使用说明

### 1. 配置邮箱信息

编辑 `pop3_client.cpp`，修改以下配置：

```cpp
const string POP3_SERVER = "pop.163.com";     // POP3 服务器地址
const int POP3_PORT = 110;                     // 明文端口
const string USERNAME = "your_email@163.com";  // 你的邮箱
const string PASSWORD = "your_password";       // 密码或授权码
```

### 2. 获取授权码（重要）

现代邮件服务商（如网易、QQ、Gmail）通常**不允许使用明文密码**，需要使用**授权码**：

#### 网易邮箱（163/126）
1. 登录网页版邮箱
2. 设置 → POP3/SMTP/IMAP
3. 开启 POP3/SMTP 服务
4. 获取授权码（通常需要手机验证）

#### QQ 邮箱
1. 登录网页版 QQ 邮箱
2. 设置 → 账户 → POP3/IMAP/SMTP 服务
3. 开启服务并生成授权码

### 3. 关于 SSL/TLS

⚠️ **本项目使用明文连接（端口 110），不支持加密！**

大多数现代邮件服务器**强制要求 SSL/TLS**（端口 995）。如果你遇到连接失败，有以下选择：

1. **测试用途**：搭建本地 POP3 测试服务器（如 `hMailServer`、`Dovecot`）
2. **学习升级**：在此基础上添加 OpenSSL/TLS 支持
3. **查找服务器**：寻找支持明文 110 端口的测试邮件服务器（非常罕见）

### 4. 运行示例

成功运行后的输出示例：

```
========================================
     POP3 客户端 - 学习演示版本
========================================

[信息] Socket 创建成功
[信息] 正在连接到 pop.163.com:110 ...
[信息] 连接成功！

--- 接收服务器欢迎消息 ---
[接收] +OK Welcome to 163 POP3 Server

--- 开始身份验证 (AUTHORIZATION 状态) ---
[发送] USER your_email@163.com
[接收] +OK
[发送] PASS ********
[接收] +OK 1 message(s) [156 byte(s)]
[信息] 登录成功！邮箱已锁定

--- 进入事务状态 (TRANSACTION 状态) ---
[1] 获取邮箱状态 (STAT 命令)
[发送] STAT
[接收] +OK 1 156
[信息] 邮箱中共有 1 封邮件

...
```

---

## 📂 代码结构

```
pop3_client.cpp
├── 头文件区域          # 跨平台 Socket 头文件
├── 配置区域          # 服务器地址、用户名、密码
├── 工具函数
│   ├── initSocket()           # 初始化 Socket 库
│   ├── sendCommand()          # 发送 POP3 命令
│   ├── receiveResponse()      # 接收单行响应
│   ├── receiveMultilineResponse()  # 接收多行响应
│   ├── isOK()                # 检查响应是否成功
│   └── parseMessageCount()    # 解析邮件数量
└── main()              # 主逻辑
    ├── Socket 初始化
    ├── 连接服务器
    ├── AUTHORIZATION 状态 (USER/PASS)
    ├── TRANSACTION 状态 (STAT/LIST/RETR/DELE)
    └── UPDATE 状态 (QUIT)
```

### 核心技术点

1. **多行响应处理**：
   - POP3 的 `LIST` 和 `RETR` 命令返回多行数据
   - 响应以单独一行的 `.\r\n` 作为结束标记

2. **状态管理**：
   - 严格遵循 POP3 的三状态模型
   - 每个状态只能执行特定的命令

3. **错误处理**：
   - 检查每个命令的响应（`+OK` / `-ERR`）
   - 提供详细的错误提示

---

## ⚠️ 重要提醒

### 安全警告

🔴 **本项目使用明文传输，不要用于生产环境！**

- 用户名和密码以明文在网络中传输
- 容易被中间人攻击截获
- 仅用于学习和测试目的

### 兼容性说明

- ✅ 支持 Linux、macOS、Windows
- ❌ 不支持 SSL/TLS 加密连接
- ❌ 不支持 APOP 认证（MD5 摘要认证）
- ❌ 不支持 SASL 认证

### 删除邮件

代码中的 `DELE` 命令默认被注释，原因：

- 邮件删除是**永久性操作**
- 只有在 `QUIT` 后才会真正删除
- 建议在理解原理后再取消注释测试

---

## ❓ 常见问题

### Q1: 连接失败怎么办？

**可能原因**：
1. 服务器要求 SSL/TLS（端口 995）
2. 服务器地址或端口错误
3. 防火墙拦截

**解决方案**：
- 检查邮件服务商的 POP3 服务器地址
- 确认服务器是否支持明文端口 110
- 临时关闭防火墙测试

### Q2: 登录失败（-ERR）怎么办？

**可能原因**：
1. 用户名或密码错误
2. 未使用授权码（对于网易、QQ 等邮箱）
3. POP3 服务未开启

**解决方案**：
- 检查用户名格式（通常是完整的邮箱地址）
- 使用授权码替代密码
- 在邮箱设置中开启 POP3 服务

### Q3: 编译错误怎么办？

**Windows 用户**：
- 确保链接了 `ws2_32.lib` 库
- 使用 Visual Studio Developer Command Prompt

**Linux/macOS 用户**：
- 确保安装了 `g++`：`sudo apt install g++` (Ubuntu)
- 检查是否有权限执行编译后的文件：`chmod +x pop3_client`

### Q4: 如何添加 SSL/TLS 支持？

这需要使用 OpenSSL 库，步骤：

1. 安装 OpenSSL 开发库
2. 修改代码，使用 `SSL_connect()` 而不是 `connect()`
3. 编译时链接 OpenSSL：`g++ -o pop3_client pop3_client.cpp -lssl -lcrypto`

---

## 📚 学习资源

### 官方文档

- [RFC 1939 - POP3 协议标准](https://www.rfc-editor.org/rfc/rfc1939.html)
- [RFC 2449 - POP3 扩展机制](https://www.rfc-editor.org/rfc/rfc2449.html)

### 相关教程

- [Socket 编程教程 - Beej's Guide](https://beej.us/guide/bgnet/)
- [TCP/IP 网络编程](https://www.tcpipguide.com/)

### 工具推荐

- **Wireshark**：网络抓包工具，可以查看 POP3 明文协议交互
- **Telnet**：可以手动测试 POP3 命令
  ```bash
  telnet pop.example.com 110
  USER username
  PASS password
  STAT
  QUIT
  ```

---

## 🎓 学习建议

### 初学者路线

1. **理解代码**：阅读代码中的中文注释，理解每个步骤
2. **运行测试**：配置好邮箱后运行程序，观察输出
3. **抓包分析**：使用 Wireshark 捕获网络包，查看实际的协议交互
4. **手动测试**：使用 Telnet 手动发送 POP3 命令，加深理解
5. **修改功能**：尝试添加新功能（如显示邮件头部、保存到文件等）

### 进阶挑战

- 🔸 添加 SSL/TLS 支持（使用 OpenSSL）
- 🔸 实现 IMAP 协议客户端（更复杂的邮件协议）
- 🔸 添加图形界面（Qt/GTK）
- 🔸 支持 MIME 解析（解码附件、HTML 邮件）
- 🔸 实现邮件发送（SMTP 协议）

---

## 📄 许可证

本项目仅供学习使用，代码开源。

## 🙏 致谢

感谢 RFC 1939 标准文档和所有开源社区的贡献者。

---

**Happy Coding! 📬**

如有问题或建议，欢迎提出 Issue 或 Pull Request。
