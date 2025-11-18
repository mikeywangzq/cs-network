# 🌐 自定义 DNS 解析器

一个完全手写的 DNS 解析器实现，类似于 `nslookup` 和 `dig` 工具。本项目从零开始实现 DNS 协议 (RFC 1035)，使用原始 UDP Socket 与 DNS 服务器通信，深入理解网络协议的底层工作原理。

---

## ✨ 项目特点

- **🔧 完全手动实现** - 不使用 `gethostbyname` 等高级系统调用
- **📦 手动构建 DNS 包** - 字节级别构建符合 RFC 1035 标准的 DNS 查询包
- **🔍 完整解析响应** - 支持 DNS 消息压缩（指针跳转）
- **📚 详细中文注释** - 每个关键步骤都有详细的说明
- **🎓 教育价值高** - 深入理解网络字节序、二进制协议、UDP 通信

---

## 📋 功能说明

### 核心功能

1. **DNS 查询包构建**
   - 12 字节 DNS 头部（Transaction ID, Flags, Counts）
   - QNAME 编码（域名标签格式）
   - QTYPE 和 QCLASS 设置

2. **UDP Socket 通信**
   - 创建 UDP Socket
   - 发送查询到 Google Public DNS (8.8.8.8:53)
   - 接收响应（支持超时）

3. **DNS 响应解析**
   - 解析 DNS 头部
   - 跳过问题部分
   - 解析回答部分（支持指针压缩 0xC0）
   - 提取 IP 地址

### 支持的记录类型

- ✅ **A 记录** (IPv4 地址)
- ✅ **CNAME 记录** (别名)
- 🔜 **AAAA 记录** (IPv6 地址) - 可扩展
- 🔜 **MX 记录** (邮件服务器) - 可扩展

---

## 🚀 快速开始

### 环境要求

- **操作系统**: Linux / macOS / WSL
- **编译器**: g++ (支持 C++11)
- **依赖**: 标准 POSIX Socket API

### 编译方式

#### 方式 1: 使用 Makefile (推荐)

```bash
cd nslookup
make
```

#### 方式 2: 直接编译

```bash
g++ -o resolver resolver.cpp -std=c++11
```

### 运行示例

```bash
# 查询 Google
./resolver google.com

# 查询百度
./resolver www.baidu.com

# 查询 GitHub
./resolver github.com
```

### 清理编译文件

```bash
make clean
```

### 运行自动测试

```bash
make test
```

---

## 📖 技术详解

### DNS 协议结构

#### 1. DNS 头部 (12 字节)

```
 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                      ID                       |  事务ID
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |  标志位
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    QDCOUNT                    |  问题数
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ANCOUNT                    |  回答数
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    NSCOUNT                    |  授权记录数
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
|                    ARCOUNT                    |  附加记录数
+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
```

**关键字段说明**:
- **ID**: 随机生成的事务ID，用于匹配请求和响应
- **Flags**:
  - `RD=1`: Recursion Desired（期望递归查询）
  - `QR=0`: Query (0=查询, 1=响应)
- **QDCOUNT**: 问题数量（通常为 1）

#### 2. QNAME 编码格式

域名需要转换为特殊的标签格式:

```
原始域名: google.com
编码后:   [6]google[3]com[0]
十六进制: 06 67 6F 6F 67 6C 65 03 63 6F 6D 00
```

**编码规则**:
- 每个标签前加长度字节
- 标签之间不用分隔符
- 以 `0x00` 结尾

#### 3. DNS 消息压缩

DNS 使用指针压缩来减少消息大小:

```
0xC00C = 1100 0000 0000 1100
         ^^                    前2位=11 表示这是一个指针
           ^^^^^^^^^^^^^^      后14位是偏移量 (12 = 0x0C)
```

**示例**:
- `0xC0 0x0C` → 指向数据包偏移量 12 的位置
- `0xC0 0x1A` → 指向数据包偏移量 26 的位置

---

## 🔬 代码结构

### 文件组织

```
nslookup/
├── resolver.cpp    # 主程序（DNS解析器核心代码）
├── Makefile        # 编译配置
└── README.md       # 本文档
```

### 核心函数

| 函数名 | 功能说明 |
|--------|----------|
| `encodeDomainName()` | 将域名编码为 DNS QNAME 格式 |
| `buildDNSQuery()` | 构建完整的 DNS 查询包 |
| `parseDomainName()` | 解析域名（支持指针压缩）|
| `parseDNSResponse()` | 解析 DNS 响应包并提取信息 |
| `main()` | 主流程：构建查询 → 发送 → 接收 → 解析 |

### 数据结构

```cpp
// DNS 头部（12字节，禁止内存对齐）
struct DNSHeader {
    uint16_t id;        // 事务ID
    uint16_t flags;     // 标志位
    uint16_t qdcount;   // 问题数
    uint16_t ancount;   // 回答数
    uint16_t nscount;   // 授权记录数
    uint16_t arcount;   // 附加记录数
} __attribute__((packed));

// DNS 问题部分（QNAME 之后的4字节）
struct DNSQuestion {
    uint16_t qtype;     // 查询类型（1=A）
    uint16_t qclass;    // 查询类（1=IN）
} __attribute__((packed));

// DNS 资源记录
struct DNSResourceRecord {
    uint16_t type;      // 记录类型
    uint16_t class_;    // 记录类
    uint32_t ttl;       // 生存时间
    uint16_t rdlength;  // 数据长度
} __attribute__((packed));
```

---

## 📊 运行示例

### 示例输出

```
$ ./resolver google.com
正在查询域名: google.com
查询包大小: 28 字节
DNS 服务器: 8.8.8.8:53
已发送 28 字节到 DNS 服务器
收到 60 字节响应

========== DNS 响应解析 ==========
事务ID: 0x1a2b
标志位: 0x8180
问题数量: 1
回答数量: 1

========== 解析结果 ==========

记录 #1:
  名称: google.com
  类型: 1 (A记录 - IPv4)
  类: 1 (IN)
  TTL: 299 秒
  IP地址: 142.250.185.46

=================================
```

---

## 🎯 学习要点

### 网络字节序

DNS 协议使用**网络字节序**（大端序），需要进行转换:

| 函数 | 用途 | 示例 |
|------|------|------|
| `htons()` | 主机序 → 网络序 (16位) | 构建查询包时使用 |
| `ntohs()` | 网络序 → 主机序 (16位) | 解析响应包时使用 |
| `htonl()` | 主机序 → 网络序 (32位) | 处理 TTL 等字段 |
| `ntohl()` | 网络序 → 主机序 (32位) | 处理 TTL 等字段 |

### 内存对齐

使用 `__attribute__((packed))` 禁止编译器自动对齐，确保结构体大小正确:

```cpp
struct DNSHeader {
    uint16_t id;
    uint16_t flags;
    ...
} __attribute__((packed));  // 精确控制内存布局
```

### UDP vs TCP

- **本项目使用 UDP** (`SOCK_DGRAM`)
- DNS 查询默认使用 UDP 端口 53
- UDP 无连接、轻量级，适合小数据包查询
- 如果响应超过 512 字节，DNS 会使用 TCP（本项目未实现）

---

## 🛠️ 故障排查

### 常见问题

1. **权限错误**

   如果遇到权限问题，可能不需要 sudo（UDP 53 端口通常不需要）:
   ```bash
   ./resolver google.com
   ```

2. **编译错误**

   确保使用 C++11 标准:
   ```bash
   g++ -o resolver resolver.cpp -std=c++11
   ```

3. **超时错误**

   可能原因:
   - 网络连接问题
   - DNS 服务器不可达
   - 防火墙阻止 UDP 53 端口

   解决方法:
   ```bash
   # 测试网络连通性
   ping 8.8.8.8

   # 使用系统 DNS 工具对比
   nslookup google.com 8.8.8.8
   ```

4. **域名不存在**

   检查 RCODE 返回码:
   - `0` = 成功
   - `3` = 域名不存在 (NXDOMAIN)

---

## 🚀 扩展方向

### 初级扩展

- [ ] 支持 IPv6 (AAAA 记录)
- [ ] 支持 MX 记录（邮件服务器）
- [ ] 支持 NS 记录（域名服务器）
- [ ] 支持 TXT 记录（文本信息）

### 中级扩展

- [ ] 支持自定义 DNS 服务器（命令行参数）
- [ ] 支持 TCP 查询（大于 512 字节的响应）
- [ ] 添加查询统计（响应时间、成功率）
- [ ] 支持 DNS over TLS (DoT)

### 高级扩展

- [ ] 实现完整的 DNS 服务器
- [ ] 支持 DNS 缓存
- [ ] 支持 DNSSEC（安全扩展）
- [ ] 支持 DNS over HTTPS (DoH)

---

## 📚 参考资料

### RFC 文档

- [RFC 1035](https://www.rfc-editor.org/rfc/rfc1035) - DNS 协议标准
- [RFC 1034](https://www.rfc-editor.org/rfc/rfc1034) - DNS 概念和设施

### 学习资源

- [DNS 查询过程详解](https://www.cloudflare.com/learning/dns/what-is-dns/)
- [Wireshark DNS 抓包分析](https://wiki.wireshark.org/DNS)
- [DNS 记录类型大全](https://en.wikipedia.org/wiki/List_of_DNS_record_types)

### 工具对比

| 工具 | 描述 | 用途 |
|------|------|------|
| `nslookup` | 简单的 DNS 查询工具 | 快速查询 IP |
| `dig` | 详细的 DNS 调试工具 | 深入分析 DNS |
| `host` | 轻量级 DNS 查询 | 脚本使用 |
| **本项目** | 自定义实现 | 学习 DNS 原理 |

---

## 🤝 贡献

欢迎提交 Issue 和 Pull Request！

### 改进建议

- 优化错误处理
- 添加更多记录类型支持
- 改进输出格式
- 添加单元测试

---

## 📜 许可证

本项目仅用于教育和学习目的。

---

## 👨‍💻 作者

网络协议学习项目 - DNS 解析器实现

---

## ⭐ 总结

这个项目通过手动实现 DNS 协议，帮助你深入理解:

1. **网络协议的二进制格式** - 如何构建和解析二进制数据包
2. **字节序转换** - 网络字节序 vs 主机字节序
3. **UDP Socket 编程** - 无连接的数据报通信
4. **DNS 协议细节** - 头部、问题、回答、压缩等
5. **C/C++ 底层编程** - 内存操作、指针、结构体对齐

通过这个项目，你将获得扎实的网络编程基础，为后续学习 TCP/IP、HTTP、TLS 等协议打下坚实基础！

---

**Happy Coding! 🎉**
