/**
 * my_traceroute.cpp - 简易路径跟踪工具 (Traceroute)
 *
 * 功能：通过逐步递增 TTL (Time-To-Live) 值，追踪数据包到达目标主机所经过的路由器路径。
 *
 * 原理：
 * 1. 发送带有递增 TTL 的 UDP 数据包到目标主机的一个不可达端口
 * 2. 每个中间路由器收到 TTL=1 的包时会丢弃并返回 ICMP "Time Exceeded" (类型 11)
 * 3. 最终目标主机会返回 ICMP "Destination Unreachable" (类型 3)，因为端口不可达
 * 4. 通过接收这些 ICMP 响应，我们可以获得每一跳路由器的 IP 地址
 *
 * 编译方法：g++ -o my_traceroute my_traceroute.cpp
 * 运行方法：sudo ./my_traceroute <目标主机>
 *
 * 注意：此程序需要 root (sudo) 权限才能创建原始套接字 (Raw Socket)！
 */

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

// 常量定义
const int MAX_HOPS = 30;           // 最大跳数，防止无限循环
const int RECV_TIMEOUT_SEC = 3;    // 接收超时时间（秒）
const int UDP_BASE_PORT = 33434;   // UDP 探测包的目标端口（通常这个端口不会被使用）
const int PACKET_SIZE = 64;        // 发送数据包的大小

/**
 * ICMP 包的结构（简化版）
 * 注意：实际的 IP 包格式为：IP头(20字节) + ICMP头(8字节) + 数据
 */
struct ICMPPacket {
    struct iphdr ip_header;        // IP 头部（20 字节）
    struct icmphdr icmp_header;    // ICMP 头部（8 字节）
    char data[PACKET_SIZE];        // 额外数据
};

/**
 * 解析主机名为 IP 地址
 *
 * @param hostname 主机名（如 "google.com"）
 * @param dest_addr 输出参数，存储解析后的地址结构
 * @return 成功返回 0，失败返回 -1
 */
int resolve_hostname(const char* hostname, struct sockaddr_in* dest_addr) {
    // 使用 gethostbyname 解析域名（传统方法，也可以用 getaddrinfo）
    struct hostent* host = gethostbyname(hostname);

    if (host == NULL) {
        cerr << "❌ 无法解析主机名: " << hostname << endl;
        cerr << "   错误信息: " << hstrerror(h_errno) << endl;
        return -1;
    }

    // 填充目标地址结构
    memset(dest_addr, 0, sizeof(*dest_addr));
    dest_addr->sin_family = AF_INET;
    dest_addr->sin_port = htons(UDP_BASE_PORT);  // 设置一个不可达的端口

    // 复制第一个 IP 地址（host->h_addr_list[0]）
    memcpy(&dest_addr->sin_addr, host->h_addr_list[0], host->h_length);

    cout << "🎯 目标主机: " << hostname << " ("
         << inet_ntoa(dest_addr->sin_addr) << ")" << endl;
    cout << "📊 最大跳数: " << MAX_HOPS << " 跳" << endl;
    cout << "⏱️  超时时间: " << RECV_TIMEOUT_SEC << " 秒\n" << endl;

    return 0;
}

/**
 * 发送一个带有指定 TTL 的 UDP 探测包
 *
 * @param dest_addr 目标地址
 * @param ttl 要设置的 TTL 值
 * @return 成功返回 0，失败返回 -1
 */
int send_probe_packet(struct sockaddr_in* dest_addr, int ttl) {
    // =====================================================================
    // 关键点 1: 创建 UDP Socket (SOCK_DGRAM)
    // =====================================================================
    int udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        cerr << "❌ 无法创建 UDP socket: " << strerror(errno) << endl;
        return -1;
    }

    // =====================================================================
    // 关键点 2: 设置 TTL (Time-To-Live) - 这是 Traceroute 的核心！
    // =====================================================================
    // IP_TTL 选项用于设置 IP 数据包的生存时间
    // 每经过一个路由器，TTL 会减 1
    // 当 TTL 减到 0 时，路由器会丢弃数据包并发送 ICMP "Time Exceeded" 消息
    if (setsockopt(udp_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        cerr << "❌ 无法设置 TTL: " << strerror(errno) << endl;
        close(udp_sock);
        return -1;
    }

    // 准备要发送的数据（内容不重要，只是为了触发 ICMP 响应）
    char send_buffer[PACKET_SIZE];
    memset(send_buffer, 0, PACKET_SIZE);
    sprintf(send_buffer, "TRACEROUTE PROBE (TTL=%d)", ttl);

    // =====================================================================
    // 发送 UDP 数据包
    // =====================================================================
    // 我们故意发送到一个"不可能"被使用的高端口号（33434）
    // 这样最终目标主机会返回 ICMP "Destination Unreachable"
    ssize_t sent = sendto(udp_sock, send_buffer, PACKET_SIZE, 0,
                          (struct sockaddr*)dest_addr, sizeof(*dest_addr));

    if (sent < 0) {
        cerr << "❌ 发送探测包失败: " << strerror(errno) << endl;
        close(udp_sock);
        return -1;
    }

    close(udp_sock);
    return 0;
}

/**
 * 接收并解析 ICMP 响应包
 *
 * @param ttl 当前的 TTL 值（用于显示）
 * @param target_ip 目标 IP 地址（用于判断是否到达终点）
 * @return 返回 0 表示继续，返回 1 表示到达目标，返回 -1 表示超时
 */
int receive_icmp_response(int ttl, struct in_addr target_ip) {
    // =====================================================================
    // 关键点 3: 创建原始套接字 (Raw Socket) 来接收 ICMP 包
    // =====================================================================
    // SOCK_RAW: 原始套接字，可以接收底层的网络协议数据
    // IPPROTO_ICMP: 指定我们要接收 ICMP 协议的数据包
    //
    // ⚠️ 重要：创建原始套接字需要 root 权限！
    // 如果程序运行时提示 "Permission denied"，请使用 sudo 运行
    int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (icmp_sock < 0) {
        cerr << "❌ 无法创建 ICMP Raw Socket: " << strerror(errno) << endl;
        cerr << "   💡 提示：此程序需要 root 权限，请使用 sudo 运行！" << endl;
        return -1;
    }

    // =====================================================================
    // 关键点 4: 设置接收超时时间
    // =====================================================================
    // 如果在指定时间内没有收到响应，recvfrom 会返回 -1 并设置 errno
    struct timeval timeout;
    timeout.tv_sec = RECV_TIMEOUT_SEC;   // 秒
    timeout.tv_usec = 0;                 // 微秒

    if (setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        cerr << "❌ 无法设置接收超时: " << strerror(errno) << endl;
        close(icmp_sock);
        return -1;
    }

    // =====================================================================
    // 接收 ICMP 响应包
    // =====================================================================
    char recv_buffer[512];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    ssize_t received = recvfrom(icmp_sock, recv_buffer, sizeof(recv_buffer), 0,
                                (struct sockaddr*)&sender_addr, &addr_len);

    close(icmp_sock);

    // =====================================================================
    // 情况 B: 超时 - 这一跳的路由器没有响应
    // =====================================================================
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            cout << ttl << "\t* * * (请求超时)" << endl;
            return -1;  // 超时，但继续下一跳
        } else {
            cerr << "❌ 接收 ICMP 响应失败: " << strerror(errno) << endl;
            return -1;
        }
    }

    // =====================================================================
    // 情况 A: 成功收到 ICMP 响应 - 解析 ICMP 包
    // =====================================================================
    // ICMP 包的结构：[IP 头部 (20字节)] + [ICMP 头部 (8字节)] + [数据]

    // 1. 解析 IP 头部（前 20 字节）
    struct iphdr* ip_hdr = (struct iphdr*)recv_buffer;

    // 2. 跳过 IP 头部，获取 ICMP 头部
    //    IP 头部长度 = ip_hdr->ihl * 4（ihl 是以 4 字节为单位的长度）
    int ip_header_len = ip_hdr->ihl * 4;
    struct icmphdr* icmp_hdr = (struct icmphdr*)(recv_buffer + ip_header_len);

    // 3. 获取发送者（路由器或目标主机）的 IP 地址
    char sender_ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sender_addr.sin_addr, sender_ip_str, INET_ADDRSTRLEN);

    // 4. 尝试反向解析 IP 地址为主机名（可选，可能会很慢）
    char hostname[256] = {0};
    struct hostent* host = gethostbyaddr(&sender_addr.sin_addr,
                                        sizeof(sender_addr.sin_addr),
                                        AF_INET);
    if (host != NULL) {
        strncpy(hostname, host->h_name, sizeof(hostname) - 1);
    } else {
        strcpy(hostname, "未知主机");
    }

    // =====================================================================
    // 关键点 5: 检查 ICMP 类型
    // =====================================================================

    // ICMP Type 11 = Time Exceeded (超时)
    // 表示 TTL 在某个路由器处减到了 0，该路由器丢弃了数据包并返回此消息
    if (icmp_hdr->type == ICMP_TIME_EXCEEDED) {
        cout << ttl << "\t" << sender_ip_str << " (" << hostname << ")" << endl;
        return 0;  // 继续下一跳
    }

    // ICMP Type 3 = Destination Unreachable (目标不可达)
    // 表示我们已经到达了目标主机（因为我们发送的端口不可达）
    else if (icmp_hdr->type == ICMP_DEST_UNREACH) {
        cout << ttl << "\t" << sender_ip_str << " (" << hostname << ") [目标已到达]" << endl;
        return 1;  // 到达目标，停止探测
    }

    // 其他 ICMP 类型（不太常见）
    else {
        cout << ttl << "\t" << sender_ip_str << " (ICMP 类型: " << (int)icmp_hdr->type << ")" << endl;
        return 0;  // 继续下一跳
    }
}

/**
 * 主函数
 */
int main(int argc, char* argv[]) {
    // =====================================================================
    // 1. 检查命令行参数
    // =====================================================================
    if (argc != 2) {
        cerr << "用法: " << argv[0] << " <目标主机>" << endl;
        cerr << "示例: sudo " << argv[0] << " google.com" << endl;
        return 1;
    }

    const char* target_hostname = argv[1];

    // =====================================================================
    // 2. 解析目标主机名为 IP 地址
    // =====================================================================
    struct sockaddr_in dest_addr;
    if (resolve_hostname(target_hostname, &dest_addr) != 0) {
        return 1;
    }

    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    cout << "开始路径跟踪...\n" << endl;

    // =====================================================================
    // 3. 主循环：逐步递增 TTL，发送探测包并接收响应
    // =====================================================================
    for (int ttl = 1; ttl <= MAX_HOPS; ttl++) {
        // 发送带有当前 TTL 的探测包
        if (send_probe_packet(&dest_addr, ttl) != 0) {
            cerr << "❌ 发送探测包失败，跳过 TTL=" << ttl << endl;
            continue;
        }

        // 接收并解析 ICMP 响应
        int result = receive_icmp_response(ttl, dest_addr.sin_addr);

        // 如果到达目标（返回值为 1），则停止探测
        if (result == 1) {
            cout << "\n✅ 成功到达目标主机！" << endl;
            break;
        }

        // 如果达到最大跳数
        if (ttl == MAX_HOPS) {
            cout << "\n⚠️  已达到最大跳数 (" << MAX_HOPS << ")，停止探测。" << endl;
        }
    }

    cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << endl;
    return 0;
}
