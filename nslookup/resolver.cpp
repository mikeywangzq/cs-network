/**
 * DNS 解析器 - 自定义 nslookup/dig 实现
 *
 * 这个程序演示了如何手动构建和解析 DNS 协议包 (RFC 1035)
 * 使用原始 UDP Socket 与 DNS 服务器通信，不依赖 gethostbyname 等系统调用
 *
 * 编译: g++ -o resolver resolver.cpp -std=c++11
 * 运行: ./resolver google.com
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

// ============================================================================
// DNS 协议数据结构定义
// ============================================================================

/**
 * DNS 头部结构 (12 字节)
 * 注意: 使用 __attribute__((packed)) 防止编译器自动对齐，确保结构体大小正确
 *
 * 字段说明:
 * - id: 事务ID，用于匹配请求和响应
 * - flags: 标志位（QR, Opcode, AA, TC, RD, RA, Z, RCODE）
 * - qdcount: 问题数量（Questions Count）
 * - ancount: 回答数量（Answer RRs Count）
 * - nscount: 授权记录数量（Authority RRs Count）
 * - arcount: 附加记录数量（Additional RRs Count）
 */
struct DNSHeader {
    uint16_t id;        // 事务ID (2字节)
    uint16_t flags;     // 标志位 (2字节)
    uint16_t qdcount;   // 问题计数 (2字节)
    uint16_t ancount;   // 回答计数 (2字节)
    uint16_t nscount;   // 授权记录计数 (2字节)
    uint16_t arcount;   // 附加记录计数 (2字节)
} __attribute__((packed));

/**
 * DNS 问题部分的尾部 (4 字节)
 * QNAME 之后紧跟的字段
 *
 * 字段说明:
 * - qtype: 查询类型 (1=A记录, 2=NS, 5=CNAME, 等等)
 * - qclass: 查询类 (1=IN互联网)
 */
struct DNSQuestion {
    uint16_t qtype;     // 查询类型 (2字节)
    uint16_t qclass;    // 查询类 (2字节)
} __attribute__((packed));

/**
 * DNS 资源记录头部 (不包括 NAME 字段)
 * 出现在 Answer, Authority, Additional 部分
 */
struct DNSResourceRecord {
    uint16_t type;      // 记录类型 (2字节)
    uint16_t class_;    // 记录类 (2字节)
    uint32_t ttl;       // 生存时间 (4字节)
    uint16_t rdlength;  // 数据长度 (2字节)
} __attribute__((packed));

// ============================================================================
// DNS 查询包构建函数
// ============================================================================

/**
 * 将域名编码为 DNS QNAME 格式
 *
 * DNS QNAME 格式说明:
 * 域名 "google.com" 编码为: [6]google[3]com[0]
 * - 每段前面加上长度字节
 * - 最后以 0x00 结尾
 *
 * 例如:
 * google.com -> \x06google\x03com\x00
 * www.example.org -> \x03www\x07example\x03org\x00
 *
 * @param domain 原始域名字符串 (例如 "google.com")
 * @param buffer 输出缓冲区
 * @return 编码后的长度
 */
int encodeDomainName(const char* domain, unsigned char* buffer) {
    unsigned char* ptr = buffer;
    const char* label_start = domain;

    // 遍历域名的每个字符
    for (const char* p = domain; ; p++) {
        // 遇到 '.' 或字符串结尾
        if (*p == '.' || *p == '\0') {
            int label_len = p - label_start;  // 计算当前标签长度

            if (label_len > 0) {
                *ptr++ = (unsigned char)label_len;  // 写入长度字节
                memcpy(ptr, label_start, label_len);  // 写入标签内容
                ptr += label_len;
            }

            // 如果到达字符串结尾，写入终止符 0x00
            if (*p == '\0') {
                *ptr++ = 0x00;
                break;
            }

            // 移动到下一个标签
            label_start = p + 1;
        }
    }

    return ptr - buffer;  // 返回总长度
}

/**
 * 构建 DNS 查询包
 *
 * DNS 查询包结构:
 * +---------------------------+
 * |   Header (12 bytes)       |  DNS头部
 * +---------------------------+
 * |   Question Section        |  问题部分
 * |   - QNAME (variable)      |    - 域名（可变长度）
 * |   - QTYPE (2 bytes)       |    - 查询类型
 * |   - QCLASS (2 bytes)      |    - 查询类
 * +---------------------------+
 *
 * @param domain 要查询的域名
 * @param buffer 输出缓冲区
 * @return 查询包的总长度
 */
int buildDNSQuery(const char* domain, unsigned char* buffer) {
    // ========================================
    // 1. 构建 DNS 头部 (12 字节)
    // ========================================
    DNSHeader* header = (DNSHeader*)buffer;

    // 生成随机事务ID (用于匹配请求和响应)
    header->id = htons(rand() % 65536);

    // 设置标志位: 0x0100
    // - QR=0 (查询)
    // - Opcode=0 (标准查询)
    // - RD=1 (期望递归查询，Recursion Desired)
    // 0x0100 = 0000 0001 0000 0000
    //          ^^^^ ^^^^ ^^^^ ^^^^
    //          QR Opcode AA TC RD RA Z RCODE
    header->flags = htons(0x0100);

    // 设置计数字段
    header->qdcount = htons(1);  // 1个问题
    header->ancount = htons(0);  // 0个回答
    header->nscount = htons(0);  // 0个授权记录
    header->arcount = htons(0);  // 0个附加记录

    // ========================================
    // 2. 构建问题部分
    // ========================================
    unsigned char* qname_ptr = buffer + sizeof(DNSHeader);

    // 编码域名为 QNAME 格式
    int qname_len = encodeDomainName(domain, qname_ptr);

    // 添加 QTYPE 和 QCLASS
    DNSQuestion* question = (DNSQuestion*)(qname_ptr + qname_len);
    question->qtype = htons(1);   // Type A (IPv4 地址)
    question->qclass = htons(1);  // Class IN (互联网)

    // 返回总长度
    int total_len = sizeof(DNSHeader) + qname_len + sizeof(DNSQuestion);
    return total_len;
}

// ============================================================================
// DNS 响应包解析函数
// ============================================================================

/**
 * 解析 DNS 响应中的域名（支持指针压缩）
 *
 * DNS 消息压缩 (RFC 1035 Section 4.1.4):
 * 为了减少消息大小，DNS 使用指针来引用之前出现过的域名
 *
 * 格式:
 * - 普通标签: 长度字节 (0-63) + 标签内容
 * - 压缩指针: 前2位为11 (0xC0) + 12位偏移量
 *
 * 例如: 0xC0 0x0C 表示指向数据包偏移量 12 (0x000C) 的位置
 *
 * @param buffer 整个DNS响应包的起始位置
 * @param pos 当前要解析的位置（会被更新）
 * @param name 输出的域名字符串
 * @return 是否解析成功
 */
bool parseDomainName(const unsigned char* buffer, int& pos, char* name) {
    int name_pos = 0;
    bool jumped = false;  // 是否使用了指针跳转
    int original_pos = pos;  // 记录原始位置
    int max_jumps = 10;  // 防止无限循环
    int jumps = 0;

    while (true) {
        // 读取长度字节
        unsigned char len = buffer[pos];

        // 情况1: 结束标志 (0x00)
        if (len == 0) {
            name[name_pos] = '\0';
            pos++;
            if (!jumped) {
                original_pos = pos;
            }
            break;
        }

        // 情况2: 指针压缩 (前2位为11，即 >= 0xC0)
        // 0xC0 = 1100 0000
        if ((len & 0xC0) == 0xC0) {
            // 读取2字节的偏移量
            // 前2位是标志位，后14位是偏移量
            int offset = ((len & 0x3F) << 8) | buffer[pos + 1];

            if (!jumped) {
                original_pos = pos + 2;  // 记录跳转前的位置
            }

            pos = offset;  // 跳转到指针指向的位置
            jumped = true;
            jumps++;

            if (jumps > max_jumps) {
                cerr << "错误: DNS 名称压缩指针循环" << endl;
                return false;
            }
            continue;
        }

        // 情况3: 普通标签
        pos++;  // 跳过长度字节

        // 添加标签分隔符 '.'
        if (name_pos > 0) {
            name[name_pos++] = '.';
        }

        // 复制标签内容
        for (int i = 0; i < len; i++) {
            name[name_pos++] = buffer[pos++];
        }
    }

    if (jumped) {
        pos = original_pos;
    }

    return true;
}

/**
 * 解析 DNS 响应包并提取 IP 地址
 *
 * DNS 响应包结构:
 * +---------------------------+
 * |   Header (12 bytes)       |
 * +---------------------------+
 * |   Question Section        |  (原样返回查询内容)
 * +---------------------------+
 * |   Answer Section          |  (包含我们需要的IP地址)
 * |   - NAME                  |
 * |   - TYPE                  |
 * |   - CLASS                 |
 * |   - TTL                   |
 * |   - RDLENGTH              |
 * |   - RDATA                 |
 * +---------------------------+
 * |   Authority Section       |  (可选)
 * +---------------------------+
 * |   Additional Section      |  (可选)
 * +---------------------------+
 *
 * @param buffer DNS响应包缓冲区
 * @param len 响应包长度
 */
void parseDNSResponse(unsigned char* buffer, int len) {
    // ========================================
    // 1. 解析 DNS 头部
    // ========================================
    DNSHeader* header = (DNSHeader*)buffer;

    // 转换字节序（网络字节序 -> 主机字节序）
    uint16_t id = ntohs(header->id);
    uint16_t flags = ntohs(header->flags);
    uint16_t qdcount = ntohs(header->qdcount);
    uint16_t ancount = ntohs(header->ancount);

    cout << "\n========== DNS 响应解析 ==========" << endl;
    cout << "事务ID: 0x" << hex << id << dec << endl;
    cout << "标志位: 0x" << hex << flags << dec << endl;

    // 检查响应码 (RCODE, 最后4位)
    int rcode = flags & 0x000F;
    if (rcode != 0) {
        cerr << "错误: DNS 查询失败，RCODE = " << rcode << endl;
        switch (rcode) {
            case 1: cerr << "  格式错误" << endl; break;
            case 2: cerr << "  服务器失败" << endl; break;
            case 3: cerr << "  域名不存在" << endl; break;
            case 4: cerr << "  不支持的查询" << endl; break;
            case 5: cerr << "  服务器拒绝" << endl; break;
        }
        return;
    }

    cout << "问题数量: " << qdcount << endl;
    cout << "回答数量: " << ancount << endl;

    if (ancount == 0) {
        cout << "没有找到 IP 地址" << endl;
        return;
    }

    // ========================================
    // 2. 跳过问题部分
    // ========================================
    // 响应包会原样返回问题部分，我们需要跳过它
    int pos = sizeof(DNSHeader);

    for (int i = 0; i < qdcount; i++) {
        char domain[256];
        if (!parseDomainName(buffer, pos, domain)) {
            cerr << "错误: 无法解析问题部分的域名" << endl;
            return;
        }

        // 跳过 QTYPE 和 QCLASS (各2字节)
        pos += 4;
    }

    // ========================================
    // 3. 解析回答部分
    // ========================================
    cout << "\n========== 解析结果 ==========" << endl;

    for (int i = 0; i < ancount; i++) {
        // 解析名称
        char name[256];
        if (!parseDomainName(buffer, pos, name)) {
            cerr << "错误: 无法解析回答部分的域名" << endl;
            return;
        }

        // 确保不会越界
        if (pos + sizeof(DNSResourceRecord) > len) {
            cerr << "错误: 响应包数据不完整" << endl;
            return;
        }

        // 解析资源记录
        DNSResourceRecord* rr = (DNSResourceRecord*)(buffer + pos);
        pos += sizeof(DNSResourceRecord);

        uint16_t type = ntohs(rr->type);
        uint16_t class_ = ntohs(rr->class_);
        uint32_t ttl = ntohl(rr->ttl);
        uint16_t rdlength = ntohs(rr->rdlength);

        // 确保不会越界
        if (pos + rdlength > len) {
            cerr << "错误: RDATA 长度超出响应包范围" << endl;
            return;
        }

        cout << "\n记录 #" << (i + 1) << ":" << endl;
        cout << "  名称: " << name << endl;
        cout << "  类型: " << type;

        // 处理 A 记录 (IPv4 地址)
        if (type == 1) {
            cout << " (A记录 - IPv4)" << endl;
            cout << "  类: " << class_ << " (IN)" << endl;
            cout << "  TTL: " << ttl << " 秒" << endl;

            if (rdlength == 4) {
                // 解析 IPv4 地址
                unsigned char* ip = buffer + pos;
                cout << "  IP地址: "
                     << (int)ip[0] << "."
                     << (int)ip[1] << "."
                     << (int)ip[2] << "."
                     << (int)ip[3] << endl;
            }
        }
        // 处理 CNAME 记录
        else if (type == 5) {
            cout << " (CNAME记录 - 别名)" << endl;
            cout << "  类: " << class_ << endl;
            cout << "  TTL: " << ttl << " 秒" << endl;

            int cname_pos = pos;
            char cname[256];
            if (parseDomainName(buffer, cname_pos, cname)) {
                cout << "  别名指向: " << cname << endl;
            }
        }
        // 其他类型
        else {
            cout << " (其他类型)" << endl;
        }

        // 移动到下一条记录
        pos += rdlength;
    }

    cout << "\n=================================" << endl;
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc != 2) {
        cerr << "用法: " << argv[0] << " <域名>" << endl;
        cerr << "示例: " << argv[0] << " google.com" << endl;
        return 1;
    }

    const char* domain = argv[1];

    // 初始化随机数生成器（用于生成事务ID）
    srand(time(NULL));

    cout << "正在查询域名: " << domain << endl;

    // ========================================
    // 1. 构建 DNS 查询包
    // ========================================
    unsigned char query_buffer[512];
    int query_len = buildDNSQuery(domain, query_buffer);

    cout << "查询包大小: " << query_len << " 字节" << endl;

    // ========================================
    // 2. 创建 UDP Socket
    // ========================================
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("创建 socket 失败");
        return 1;
    }

    // 设置接收超时（5秒）
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("设置超时失败");
        close(sockfd);
        return 1;
    }

    // ========================================
    // 3. 设置 DNS 服务器地址 (Google Public DNS: 8.8.8.8:53)
    // ========================================
    struct sockaddr_in dns_server;
    memset(&dns_server, 0, sizeof(dns_server));
    dns_server.sin_family = AF_INET;
    dns_server.sin_port = htons(53);  // DNS 端口 53

    // 将 IP 地址字符串转换为二进制形式
    if (inet_pton(AF_INET, "8.8.8.8", &dns_server.sin_addr) <= 0) {
        perror("无效的 DNS 服务器地址");
        close(sockfd);
        return 1;
    }

    cout << "DNS 服务器: 8.8.8.8:53" << endl;

    // ========================================
    // 4. 发送 DNS 查询
    // ========================================
    ssize_t sent = sendto(sockfd, query_buffer, query_len, 0,
                         (struct sockaddr*)&dns_server, sizeof(dns_server));

    if (sent < 0) {
        perror("发送查询失败");
        close(sockfd);
        return 1;
    }

    cout << "已发送 " << sent << " 字节到 DNS 服务器" << endl;

    // ========================================
    // 5. 接收 DNS 响应
    // ========================================
    unsigned char response_buffer[512];
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);

    ssize_t received = recvfrom(sockfd, response_buffer, sizeof(response_buffer), 0,
                               (struct sockaddr*)&from_addr, &from_len);

    if (received < 0) {
        perror("接收响应失败（可能超时）");
        close(sockfd);
        return 1;
    }

    cout << "收到 " << received << " 字节响应" << endl;

    // ========================================
    // 6. 解析 DNS 响应
    // ========================================
    parseDNSResponse(response_buffer, received);

    // 关闭 socket
    close(sockfd);

    return 0;
}
