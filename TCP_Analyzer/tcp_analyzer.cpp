/*
 * TCP 协议分析器 - 有状态的连接跟踪器
 *
 * 功能：捕获网络数据包，解析 TCP 协议，跟踪每个连接的状态转换
 * 平台：Linux (使用 AF_PACKET 原始套接字)
 * 编译：g++ -o tcp_analyzer tcp_analyzer.cpp
 * 运行：sudo ./tcp_analyzer <interface>
 */

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <map>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <sys/time.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

// ======================== 协议头部结构定义 ========================

/*
 * 注意：本程序使用 Linux 系统提供的协议头部结构：
 * - struct ethhdr: 在 <linux/if_ether.h> 中定义（以太网头部）
 * - struct iphdr: 在 <netinet/ip.h> 中定义（IPv4 头部）
 * - struct tcphdr: 在 <netinet/tcp.h> 中定义（TCP 头部）
 *
 * 以太网帧头部结构 (Layer 2) - 总长度: 14 字节
 *   - h_dest[6]: 目标 MAC 地址
 *   - h_source[6]: 源 MAC 地址
 *   - h_proto: 协议类型 (0x0800 = IPv4)
 *
 * IPv4 头部结构 (Layer 3) - 最小长度: 20 字节
 *   - ihl: IP头部长度 (4 bits, 以 4 字节为单位)
 *   - version: IP版本 (4 bits, IPv4 = 4)
 *   - protocol: 上层协议 (6 = TCP, 17 = UDP, 1 = ICMP)
 *   - saddr/daddr: 源/目标 IP 地址
 *
 * TCP 头部结构 (Layer 4) - 最小长度: 20 字节
 *   - source/dest: 源/目标端口号
 *   - seq/ack_seq: 序列号/确认号
 *   - 标志位: syn, ack, fin, rst, psh, urg
 *   - doff: TCP头部长度 (4 bits, 以 4 字节为单位)
 */

// ======================== TCP 状态机定义 ========================

/*
 * TCP 连接状态枚举
 * 这是一个简化的 TCP 状态机，实际 TCP 有 11 个状态
 *
 * 完整的 TCP 状态机包括:
 * CLOSED -> LISTEN -> SYN_RCVD -> ESTABLISHED ->
 * FIN_WAIT_1 -> FIN_WAIT_2 -> TIME_WAIT -> CLOSED
 * 或者: CLOSE_WAIT -> LAST_ACK -> CLOSED
 */
enum TcpState {
    CLOSED,          // 初始状态，连接不存在
    SYN_SENT,        // 客户端发送 SYN，等待 SYN-ACK
    SYN_RECEIVED,    // 服务器收到 SYN，发送 SYN-ACK，等待 ACK
    ESTABLISHED,     // 连接已建立，可以传输数据
    FIN_WAIT_1,      // 主动关闭方发送 FIN，等待 ACK 或对方的 FIN
    FIN_WAIT_2,      // 主动关闭方收到 ACK，等待对方的 FIN
    CLOSE_WAIT,      // 被动关闭方收到 FIN，发送 ACK，等待应用层关闭
    LAST_ACK,        // 被动关闭方发送 FIN，等待最后的 ACK
    TIME_WAIT,       // 主动关闭方收到对方的 FIN，等待 2MSL
    CLOSING          // 双方同时关闭
};

/*
 * 将 TCP 状态转换为可读字符串
 */
const char* state_to_string(TcpState state) {
    switch(state) {
        case CLOSED:       return "CLOSED";
        case SYN_SENT:     return "SYN_SENT";
        case SYN_RECEIVED: return "SYN_RECEIVED";
        case ESTABLISHED:  return "ESTABLISHED";
        case FIN_WAIT_1:   return "FIN_WAIT_1";
        case FIN_WAIT_2:   return "FIN_WAIT_2";
        case CLOSE_WAIT:   return "CLOSE_WAIT";
        case LAST_ACK:     return "LAST_ACK";
        case TIME_WAIT:    return "TIME_WAIT";
        case CLOSING:      return "CLOSING";
        default:           return "UNKNOWN";
    }
}

// ======================== 连接标识符 (Connection ID) ========================

/*
 * 连接标识符结构
 * 用于唯一标识一个 TCP 连接
 *
 * 注意：TCP 连接是双向的，(A->B) 和 (B->A) 应该被视为同一个连接
 * 因此我们需要"规范化" (canonicalize) 这个结构，确保无论数据包方向如何，
 * 都能映射到同一个 map key
 */
struct ConnectionID {
    uint32_t src_ip;     // 源 IP 地址
    uint32_t dst_ip;     // 目标 IP 地址
    uint16_t src_port;   // 源端口号
    uint16_t dst_port;   // 目标端口号

    // 重载 < 运算符，用于在 std::map 中作为 key
    bool operator<(const ConnectionID& other) const {
        if (src_ip != other.src_ip) return src_ip < other.src_ip;
        if (dst_ip != other.dst_ip) return dst_ip < other.dst_ip;
        if (src_port != other.src_port) return src_port < other.src_port;
        return dst_port < other.dst_port;
    }
};

/*
 * 连接规范化 (Canonicalization) 函数
 *
 * 目的：确保 (A, B) 和 (B, A) 映射到相同的 ConnectionID
 *
 * 策略：
 * 1. 比较 IP 地址，较小的作为 src_ip
 * 2. 如果 IP 相同，比较端口号，较小的作为 src_port
 *
 * 例子：
 * - 数据包1: 192.168.1.100:8080 -> 10.0.0.1:80
 *   规范化后: 10.0.0.1:80 <-> 192.168.1.100:8080
 *
 * - 数据包2: 10.0.0.1:80 -> 192.168.1.100:8080
 *   规范化后: 10.0.0.1:80 <-> 192.168.1.100:8080
 *
 * 两个数据包会映射到同一个 ConnectionID
 */
ConnectionID make_canonical_id(uint32_t ip1, uint16_t port1,
                                uint32_t ip2, uint16_t port2) {
    ConnectionID id;

    // 规范化策略：较小的 IP 作为 src_ip
    if (ip1 < ip2) {
        id.src_ip = ip1;
        id.src_port = port1;
        id.dst_ip = ip2;
        id.dst_port = port2;
    }
    else if (ip1 > ip2) {
        id.src_ip = ip2;
        id.src_port = port2;
        id.dst_ip = ip1;
        id.dst_port = port1;
    }
    else {
        // IP 地址相同，比较端口号
        if (port1 < port2) {
            id.src_ip = ip1;
            id.src_port = port1;
            id.dst_ip = ip2;
            id.dst_port = port2;
        } else {
            id.src_ip = ip2;
            id.src_port = port2;
            id.dst_ip = ip1;
            id.dst_port = port1;
        }
    }

    return id;
}

// ======================== 全局连接跟踪表 ========================

/*
 * 连接跟踪器 (Connection Tracker)
 *
 * 这是整个程序的核心数据结构：
 * - Key: 规范化的 ConnectionID (确保双向数据包映射到同一个连接)
 * - Value: 当前的 TCP 状态
 *
 * 作用：
 * 1. 记录每个 TCP 连接的当前状态
 * 2. 根据接收到的 TCP 标志位更新状态
 * 3. 检测连接的建立、数据传输、关闭过程
 */
std::map<ConnectionID, TcpState> connection_tracker;

// ======================== 辅助函数 ========================

/*
 * 将 IPv4 地址转换为可读的字符串格式
 * 输入：网络字节序的 32 位整数
 * 输出："xxx.xxx.xxx.xxx" 格式的字符串
 */
std::string ip_to_string(uint32_t ip) {
    struct in_addr addr;
    addr.s_addr = ip;
    return std::string(inet_ntoa(addr));
}

/*
 * 获取当前时间戳（秒.毫秒格式）
 * 用于在输出中显示每个事件的发生时间
 */
double get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// 程序启动时间，用于计算相对时间
double start_time = 0.0;

/*
 * 获取相对于程序启动的时间（秒）
 */
double get_relative_time() {
    return get_timestamp() - start_time;
}

// ======================== TCP 状态机处理逻辑 ========================

/*
 * 处理 TCP 数据包并更新状态机
 *
 * 参数：
 * - key: 规范化的连接标识符
 * - tcp: TCP 头部指针
 * - src_ip, dst_ip: 源和目标 IP 地址
 * - src_port, dst_port: 源和目标端口号
 * - data_len: TCP 数据部分的长度
 *
 * 这个函数实现了简化的 TCP 状态机，根据当前状态和接收到的标志位
 * 决定状态转换，并输出相应的事件信息
 */
void process_tcp_packet(ConnectionID key, struct tcphdr* tcp,
                        uint32_t src_ip, uint32_t dst_ip,
                        uint16_t src_port, uint16_t dst_port,
                        int data_len) {

    // 获取当前连接的状态（如果不存在，默认为 CLOSED）
    TcpState current_state = CLOSED;
    auto it = connection_tracker.find(key);
    if (it != connection_tracker.end()) {
        current_state = it->second;
    }

    std::string src_ip_str = ip_to_string(src_ip);
    std::string dst_ip_str = ip_to_string(dst_ip);
    double timestamp = get_relative_time();

    // ==================== RST 处理 ====================
    /*
     * RST (Reset) 标志：立即终止连接
     * 任何状态下收到 RST 都应该删除连接记录
     */
    if (tcp->rst) {
        connection_tracker.erase(key);
        printf("[%.3f] 🔴 连接重置 (RST): %s:%d <-> %s:%d [%s -> CLOSED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port),
               state_to_string(current_state));
        return;
    }

    // ==================== 三次握手：连接建立 ====================

    /*
     * 状态转换 1: CLOSED -> SYN_SENT
     * 触发条件：收到 SYN 标志，且没有 ACK 标志
     * 含义：客户端发起连接请求（三次握手的第一步）
     */
    if (current_state == CLOSED && tcp->syn && !tcp->ack) {
        connection_tracker[key] = SYN_SENT;
        printf("[%.3f] 🟢 新连接发起 (SYN): %s:%d -> %s:%d [CLOSED -> SYN_SENT]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 2: SYN_SENT -> ESTABLISHED
     * 触发条件：收到 SYN + ACK 标志
     * 含义：服务器响应连接请求（三次握手的第二步）
     *
     * 注意：这是简化模型，实际上应该先转到 SYN_RECEIVED，
     * 然后等待最后的 ACK 才转到 ESTABLISHED
     */
    if (current_state == SYN_SENT && tcp->syn && tcp->ack) {
        connection_tracker[key] = ESTABLISHED;
        printf("[%.3f] 🟢 连接建立 (SYN-ACK): %s:%d <-> %s:%d [SYN_SENT -> ESTABLISHED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 2b: SYN_SENT -> ESTABLISHED (收到最后的 ACK)
     * 触发条件：当前状态是 SYN_SENT，只有 ACK 标志
     * 含义：三次握手的第三步，客户端确认服务器的 SYN-ACK
     */
    if (current_state == SYN_SENT && tcp->ack && !tcp->syn && !tcp->fin) {
        connection_tracker[key] = ESTABLISHED;
        printf("[%.3f] 🟢 连接确认 (ACK): %s:%d <-> %s:%d [SYN_SENT -> ESTABLISHED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    // ==================== 数据传输阶段 ====================

    /*
     * 数据传输：ESTABLISHED 状态下，有数据负载
     * 触发条件：连接已建立，且 TCP 数据部分长度 > 0
     */
    if (current_state == ESTABLISHED && data_len > 0) {
        printf("[%.3f] 📦 数据传输: %s:%d -> %s:%d (%d bytes) [ESTABLISHED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port),
               data_len);
        return;
    }

    // ==================== 四次挥手：连接关闭 ====================

    /*
     * 状态转换 3: ESTABLISHED -> FIN_WAIT_1
     * 触发条件：收到 FIN 标志
     * 含义：主动关闭方发起关闭请求（四次挥手的第一步）
     */
    if (current_state == ESTABLISHED && tcp->fin) {
        connection_tracker[key] = FIN_WAIT_1;
        printf("[%.3f] 🔵 连接关闭发起 (FIN): %s:%d -> %s:%d [ESTABLISHED -> FIN_WAIT_1]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 4: FIN_WAIT_1 -> FIN_WAIT_2
     * 触发条件：收到 ACK（对 FIN 的确认）
     * 含义：对方确认了我方的关闭请求（四次挥手的第二步）
     */
    if (current_state == FIN_WAIT_1 && tcp->ack && !tcp->fin) {
        connection_tracker[key] = FIN_WAIT_2;
        printf("[%.3f] 🔵 关闭确认 (ACK): %s:%d <-> %s:%d [FIN_WAIT_1 -> FIN_WAIT_2]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 5: FIN_WAIT_1 -> CLOSING (同时关闭)
     * 触发条件：在 FIN_WAIT_1 状态下收到对方的 FIN
     * 含义：双方同时发起关闭
     */
    if (current_state == FIN_WAIT_1 && tcp->fin) {
        connection_tracker[key] = CLOSING;
        printf("[%.3f] 🔵 同时关闭 (FIN): %s:%d <-> %s:%d [FIN_WAIT_1 -> CLOSING]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 6: FIN_WAIT_2 -> TIME_WAIT
     * 触发条件：收到对方的 FIN（四次挥手的第三步）
     * 含义：对方也发起关闭，进入等待状态
     */
    if (current_state == FIN_WAIT_2 && tcp->fin) {
        connection_tracker[key] = TIME_WAIT;
        printf("[%.3f] 🔵 对方关闭 (FIN): %s:%d <-> %s:%d [FIN_WAIT_2 -> TIME_WAIT]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 7: TIME_WAIT -> CLOSED
     * 触发条件：收到最后的 ACK（四次挥手的第四步）
     * 含义：连接完全关闭
     */
    if (current_state == TIME_WAIT && tcp->ack) {
        connection_tracker.erase(key);
        printf("[%.3f] 🔵 连接完全关闭 (ACK): %s:%d <-> %s:%d [TIME_WAIT -> CLOSED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 8: CLOSING -> CLOSED
     * 触发条件：在同时关闭状态下收到 ACK
     */
    if (current_state == CLOSING && tcp->ack) {
        connection_tracker.erase(key);
        printf("[%.3f] 🔵 连接完全关闭 (ACK): %s:%d <-> %s:%d [CLOSING -> CLOSED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    // ==================== 被动关闭方的状态转换 ====================

    /*
     * 状态转换 9: ESTABLISHED -> CLOSE_WAIT
     * 触发条件：被动方收到对方的 FIN
     */
    if (current_state == ESTABLISHED && tcp->fin) {
        connection_tracker[key] = CLOSE_WAIT;
        printf("[%.3f] 🔵 收到关闭请求 (FIN): %s:%d <-> %s:%d [ESTABLISHED -> CLOSE_WAIT]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 10: CLOSE_WAIT -> LAST_ACK
     * 触发条件：被动方也发起关闭（发送 FIN）
     */
    if (current_state == CLOSE_WAIT && tcp->fin) {
        connection_tracker[key] = LAST_ACK;
        printf("[%.3f] 🔵 被动关闭 (FIN): %s:%d -> %s:%d [CLOSE_WAIT -> LAST_ACK]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }

    /*
     * 状态转换 11: LAST_ACK -> CLOSED
     * 触发条件：收到对最后一个 FIN 的 ACK
     */
    if (current_state == LAST_ACK && tcp->ack) {
        connection_tracker.erase(key);
        printf("[%.3f] 🔵 连接完全关闭 (ACK): %s:%d <-> %s:%d [LAST_ACK -> CLOSED]\n",
               timestamp,
               src_ip_str.c_str(), ntohs(src_port),
               dst_ip_str.c_str(), ntohs(dst_port));
        return;
    }
}

// ======================== 主程序 ========================

int main(int argc, char* argv[]) {
    // 检查命令行参数
    if (argc < 2) {
        std::cerr << "用法: sudo " << argv[0] << " <网络接口名>\n";
        std::cerr << "例如: sudo " << argv[0] << " eth0\n";
        std::cerr << "      sudo " << argv[0] << " wlan0\n";
        return 1;
    }

    const char* interface = argv[1];

    // 记录程序启动时间
    start_time = get_timestamp();

    printf("====================================================\n");
    printf("      TCP 协议分析器 - 有状态连接跟踪器\n");
    printf("====================================================\n");
    printf("监听接口: %s\n", interface);
    printf("开始时间: %.3f\n", start_time);
    printf("====================================================\n\n");

    /*
     * 创建原始套接字 (Raw Socket)
     *
     * AF_PACKET: 工作在数据链路层，可以捕获所有以太网帧
     * SOCK_RAW: 原始套接字，获取完整的数据包（包括头部）
     * htons(ETH_P_ALL): 捕获所有协议类型的数据包
     */
    int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (sock < 0) {
        perror("创建套接字失败 (需要 root 权限)");
        return 1;
    }

    /*
     * 绑定到特定的网络接口
     *
     * 如果不绑定，会接收所有接口的数据包
     */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);

    // 获取接口索引
    if (ioctl(sock, SIOCGIFINDEX, &ifr) < 0) {
        perror("获取接口索引失败");
        close(sock);
        return 1;
    }

    // 绑定套接字到接口
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifr.ifr_ifindex;
    sll.sll_protocol = htons(ETH_P_ALL);

    if (bind(sock, (struct sockaddr*)&sll, sizeof(sll)) < 0) {
        perror("绑定套接字失败");
        close(sock);
        return 1;
    }

    printf("✅ 套接字创建成功，开始捕获数据包...\n\n");

    // 数据包缓冲区 (65536 字节足够容纳最大的以太网帧)
    unsigned char buffer[65536];

    /*
     * 主循环：持续捕获和处理数据包
     */
    while (true) {
        // 接收一个数据包
        ssize_t packet_size = recv(sock, buffer, sizeof(buffer), 0);
        if (packet_size < 0) {
            perror("接收数据包失败");
            continue;
        }

        // ==================== Layer 2: 解析以太网头部 ====================
        struct ethhdr* eth = (struct ethhdr*)buffer;

        // 检查是否为 IPv4 数据包 (EtherType = 0x0800)
        if (ntohs(eth->h_proto) != 0x0800) {
            continue;  // 跳过非 IPv4 数据包（如 ARP, IPv6 等）
        }

        // ==================== Layer 3: 解析 IP 头部 ====================
        struct iphdr* ip = (struct iphdr*)(buffer + sizeof(struct ethhdr));

        // 检查是否为 TCP 数据包 (Protocol = 6)
        if (ip->protocol != 6) {
            continue;  // 跳过非 TCP 数据包（如 UDP, ICMP 等）
        }

        // ==================== Layer 4: 解析 TCP 头部 ====================

        /*
         * 计算 TCP 头部的偏移量
         *
         * TCP 头部位置 = 以太网头部 + IP 头部
         * IP 头部长度 = ip->ihl * 4 (ihl 以 4 字节为单位)
         */
        int ip_header_len = ip->ihl * 4;
        struct tcphdr* tcp = (struct tcphdr*)(buffer + sizeof(struct ethhdr) + ip_header_len);

        // 提取连接信息
        uint32_t src_ip = ip->saddr;
        uint32_t dst_ip = ip->daddr;
        uint16_t src_port = tcp->source;
        uint16_t dst_port = tcp->dest;

        /*
         * 计算 TCP 数据部分的长度
         *
         * TCP 数据长度 = IP 总长度 - IP 头部长度 - TCP 头部长度
         * TCP 头部长度 = tcp->doff * 4 (doff 以 4 字节为单位)
         */
        int tcp_header_len = tcp->doff * 4;
        int ip_total_len = ntohs(ip->tot_len);
        int tcp_data_len = ip_total_len - ip_header_len - tcp_header_len;

        // ==================== 连接规范化 ====================
        /*
         * 将 (src, dst) 规范化为统一的连接标识符
         * 这样无论数据包方向如何，都能映射到同一个连接记录
         */
        ConnectionID key = make_canonical_id(src_ip, ntohs(src_port),
                                             dst_ip, ntohs(dst_port));

        // ==================== 状态机处理 ====================
        /*
         * 调用状态机处理函数
         * 根据当前状态和 TCP 标志位，更新连接状态并输出事件信息
         */
        process_tcp_packet(key, tcp, src_ip, dst_ip, src_port, dst_port, tcp_data_len);
    }

    // 关闭套接字（实际上这里永远不会执行，因为是无限循环）
    close(sock);
    return 0;
}
