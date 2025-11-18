/*
 * ===================================================================
 * my_ping.cpp - 极简ICMP Ping工具 (原始套接字实现)
 * ===================================================================
 *
 * 功能说明：
 *   使用原始套接字(SOCK_RAW)和ICMP协议实现一个简单的ping工具
 *   能够发送ICMP Echo Request并接收Echo Reply
 *
 * 编译方法：
 *   g++ -o my_ping my_ping.cpp
 *
 * 运行方法：
 *   sudo ./my_ping <目标主机名或IP>
 *   例如: sudo ./my_ping google.com
 *
 * 注意事项：
 *   必须使用sudo运行，因为创建原始套接字需要root权限
 *
 * ===================================================================
 */

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <cerrno>

using namespace std;

// ===================================================================
// 定义常量
// ===================================================================
#define PACKET_SIZE 64           // ICMP数据包大小
#define RECV_TIMEOUT 3           // 接收超时时间（秒）

// ===================================================================
// ICMP校验和计算函数 (16位Internet Checksum)
// ===================================================================
/*
 * 校验和算法说明：
 * 1. 将数据视为16位字(uint16_t)的序列
 * 2. 将所有16位字相加（使用32位累加器防止溢出）
 * 3. 将溢出的高16位加回到低16位（处理进位）
 * 4. 将结果取反（~操作符）
 *
 * 这是标准的Internet校验和算法，用于IP、ICMP、TCP、UDP等协议
 */
unsigned short checksum(void *data, int len) {
    unsigned short *buf = (unsigned short *)data;
    unsigned int sum = 0;

    // 将数据按16位字累加
    for (int i = 0; i < len / 2; i++) {
        sum += buf[i];
    }

    // 如果数据长度为奇数，需要处理最后一个字节
    if (len % 2 != 0) {
        sum += *(unsigned char *)(&buf[len / 2]);
    }

    // 将溢出的高16位加回到低16位
    // 这个循环最多执行两次（因为加一次最多产生一次新的溢出）
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // 返回校验和的反码
    return (unsigned short)(~sum);
}

// ===================================================================
// 主机名解析函数
// ===================================================================
/*
 * 使用gethostbyname将主机名解析为IP地址
 * 参数：
 *   hostname - 主机名（例如: google.com）
 *   ip_str   - 用于存储解析后的IP地址字符串
 * 返回值：
 *   成功返回sockaddr_in结构，失败则退出程序
 */
struct sockaddr_in resolve_hostname(const char *hostname, char *ip_str) {
    struct sockaddr_in addr;
    struct hostent *host_entity;

    cout << "[信息] 正在解析主机名: " << hostname << endl;

    // 使用gethostbyname解析主机名
    host_entity = gethostbyname(hostname);

    if (host_entity == NULL) {
        cerr << "[错误] 无法解析主机名: " << hostname << endl;
        exit(1);
    }

    // 填充sockaddr_in结构
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = 0;  // ICMP不使用端口
    addr.sin_addr = *(struct in_addr *)host_entity->h_addr;

    // 将IP地址转换为字符串形式
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);

    cout << "[信息] 解析成功: " << hostname << " -> " << ip_str << endl;

    return addr;
}

// ===================================================================
// 主函数
// ===================================================================
int main(int argc, char *argv[]) {
    // 检查命令行参数
    if (argc != 2) {
        cerr << "使用方法: sudo " << argv[0] << " <目标主机名或IP>" << endl;
        cerr << "例如: sudo " << argv[0] << " google.com" << endl;
        return 1;
    }

    const char *target_host = argv[1];
    char target_ip_str[INET_ADDRSTRLEN];

    // ===============================================================
    // 步骤1: 解析目标主机名
    // ===============================================================
    struct sockaddr_in target_addr = resolve_hostname(target_host, target_ip_str);

    // ===============================================================
    // 步骤2: 创建原始套接字 (SOCK_RAW)
    // ===============================================================
    /*
     * socket参数说明：
     * - AF_INET: IPv4协议族
     * - SOCK_RAW: 原始套接字，允许直接访问底层协议
     * - IPPROTO_ICMP: ICMP协议
     *
     * 原始套接字的特点：
     * 1. 需要root权限才能创建
     * 2. 可以接收和发送自定义的ICMP数据包
     * 3. 接收到的数据包包含完整的IP头部
     */
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    if (sock < 0) {
        cerr << "[错误] 无法创建原始套接字 (是否使用了sudo?)" << endl;
        cerr << "[错误] " << strerror(errno) << endl;
        return 1;
    }

    cout << "[成功] 原始套接字创建成功 (fd=" << sock << ")" << endl;

    // ===============================================================
    // 步骤3: 设置接收超时
    // ===============================================================
    /*
     * 使用setsockopt设置SO_RCVTIMEO选项
     * 这样recvfrom在指定时间内没有收到数据时会返回错误
     */
    struct timeval tv;
    tv.tv_sec = RECV_TIMEOUT;
    tv.tv_usec = 0;

    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        cerr << "[错误] 设置接收超时失败" << endl;
        close(sock);
        return 1;
    }

    cout << "[信息] 接收超时设置为 " << RECV_TIMEOUT << " 秒" << endl;
    cout << endl;
    cout << "===== 开始Ping " << target_ip_str << " =====" << endl;
    cout << endl;

    // ===============================================================
    // 步骤4: 主循环 - 发送和接收ICMP包
    // ===============================================================
    for (int seq = 0; ; seq++) {
        // -------------------------------------------------------
        // 4A: 构建ICMP Echo Request数据包
        // -------------------------------------------------------
        char packet_buffer[PACKET_SIZE];
        memset(packet_buffer, 0, PACKET_SIZE);

        /*
         * ICMP头部结构说明 (icmphdr定义在 netinet/ip_icmp.h):
         *
         * struct icmphdr {
         *   uint8_t  type;        // ICMP类型 (8=Echo Request, 0=Echo Reply)
         *   uint8_t  code;        // ICMP代码 (通常为0)
         *   uint16_t checksum;    // 校验和
         *   union {
         *     struct {
         *       uint16_t id;      // 标识符 (用于匹配请求和响应)
         *       uint16_t sequence; // 序列号
         *     } echo;
         *   } un;
         * };
         */
        struct icmphdr *icmp_hdr = (struct icmphdr *)packet_buffer;

        // 设置ICMP类型为Echo Request
        icmp_hdr->type = ICMP_ECHO;  // 8

        // 设置ICMP代码为0
        icmp_hdr->code = 0;

        // 使用进程ID作为标识符，用于区分不同的ping进程
        icmp_hdr->un.echo.id = htons(getpid());

        // 设置序列号（每次发送递增）
        icmp_hdr->un.echo.sequence = htons(seq);

        // 先将校验和字段清零
        icmp_hdr->checksum = 0;

        // 在数据部分填充一些内容（可选）
        const char *data = "PING_DATA";
        memcpy(packet_buffer + sizeof(struct icmphdr), data, strlen(data));

        // -------------------------------------------------------
        // 4B: 计算并填充校验和
        // -------------------------------------------------------
        /*
         * 校验和的计算必须在所有其他字段填充完毕后进行
         * 校验和覆盖整个ICMP数据包（包括头部和数据部分）
         */
        icmp_hdr->checksum = checksum(packet_buffer, PACKET_SIZE);

        // -------------------------------------------------------
        // 4C: 发送ICMP Echo Request
        // -------------------------------------------------------
        /*
         * sendto函数说明：
         * - sock: 套接字描述符
         * - packet_buffer: 要发送的数据
         * - PACKET_SIZE: 数据大小
         * - 0: 标志（通常为0）
         * - (struct sockaddr *)&target_addr: 目标地址
         * - sizeof(target_addr): 地址结构大小
         */
        struct timeval send_time;
        gettimeofday(&send_time, NULL);  // 记录发送时间

        ssize_t sent_bytes = sendto(sock, packet_buffer, PACKET_SIZE, 0,
                                    (struct sockaddr *)&target_addr,
                                    sizeof(target_addr));

        if (sent_bytes < 0) {
            cerr << "[错误] 发送ICMP包失败: " << strerror(errno) << endl;
            break;
        }

        // -------------------------------------------------------
        // 4D: 接收ICMP Echo Reply
        // -------------------------------------------------------
        /*
         * 接收缓冲区必须足够大，能够容纳IP头部 + ICMP数据包
         * IP头部通常为20字节，ICMP数据包为我们发送的大小
         */
        char receive_buffer[512];
        struct sockaddr_in recv_addr;
        socklen_t addr_len = sizeof(recv_addr);

        /*
         * recvfrom函数说明：
         * - sock: 套接字描述符
         * - receive_buffer: 接收缓冲区
         * - sizeof(receive_buffer): 缓冲区大小
         * - 0: 标志（通常为0）
         * - (struct sockaddr *)&recv_addr: 源地址（输出参数）
         * - &addr_len: 地址结构大小（输入/输出参数）
         *
         * 返回值：
         * - 成功: 接收到的字节数
         * - 失败: -1 (如果设置了超时，超时也会返回-1)
         */
        ssize_t recv_bytes = recvfrom(sock, receive_buffer, sizeof(receive_buffer),
                                      0, (struct sockaddr *)&recv_addr, &addr_len);

        struct timeval recv_time;
        gettimeofday(&recv_time, NULL);  // 记录接收时间

        if (recv_bytes < 0) {
            // 接收超时或其他错误
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                cout << "[超时] 序列号 " << seq << ": 请求超时 (Request timed out)" << endl;
            } else {
                cerr << "[错误] 接收失败: " << strerror(errno) << endl;
            }
            sleep(1);
            continue;
        }

        // -------------------------------------------------------
        // 4E: 解析响应包
        // -------------------------------------------------------
        /*
         * 重要: recvfrom接收到的是完整的IP数据包!
         *
         * 数据包结构:
         * +----------------+
         * |   IP头部       |  20字节 (通常)
         * +----------------+
         * |   ICMP头部     |  8字节
         * +----------------+
         * |   ICMP数据     |  可变长度
         * +----------------+
         *
         * 我们需要：
         * 1. 先解析IP头部，获取IP头部长度
         * 2. 跳过IP头部，找到ICMP头部的位置
         * 3. 解析ICMP头部，验证是否是我们期待的响应
         */

        // 解析IP头部
        struct iphdr *ip_hdr = (struct iphdr *)receive_buffer;

        /*
         * IP头部长度字段(ihl)的单位是32位字(4字节)
         * 因此实际长度 = ihl * 4
         * 通常情况下ihl=5，即IP头部长度为20字节
         */
        int ip_header_len = ip_hdr->ihl * 4;

        // 跳过IP头部，找到ICMP头部
        struct icmphdr *recv_icmp_hdr = (struct icmphdr *)(receive_buffer + ip_header_len);

        // -------------------------------------------------------
        // 4F: 验证ICMP响应
        // -------------------------------------------------------
        /*
         * 验证步骤：
         * 1. 检查ICMP类型是否为Echo Reply (0)
         * 2. 检查ID是否匹配（是否是回复给我们的）
         * 3. 检查序列号是否匹配（是否是回复刚才发送的包）
         */
        bool is_valid_reply = true;

        // 检查1: ICMP类型
        if (recv_icmp_hdr->type != ICMP_ECHOREPLY) {  // 0
            // 收到的不是Echo Reply
            cout << "[警告] 收到非Echo Reply的ICMP包 (类型="
                 << (int)recv_icmp_hdr->type << ")" << endl;
            is_valid_reply = false;
        }

        // 检查2: ID匹配
        if (ntohs(recv_icmp_hdr->un.echo.id) != getpid()) {
            // 这个响应不是给我们的
            is_valid_reply = false;
        }

        // 检查3: 序列号匹配
        if (ntohs(recv_icmp_hdr->un.echo.sequence) != seq) {
            // 序列号不匹配（可能是之前的包的延迟响应）
            is_valid_reply = false;
        }

        // -------------------------------------------------------
        // 4G: 输出结果
        // -------------------------------------------------------
        if (is_valid_reply) {
            // 计算往返时间 (RTT - Round Trip Time)
            double rtt = (recv_time.tv_sec - send_time.tv_sec) * 1000.0 +
                        (recv_time.tv_usec - send_time.tv_usec) / 1000.0;

            char reply_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &recv_addr.sin_addr, reply_ip, INET_ADDRSTRLEN);

            cout << "[成功] 来自 " << reply_ip
                 << " 的回复: icmp_seq=" << seq
                 << " ttl=" << (int)ip_hdr->ttl
                 << " 时间=" << rtt << " ms"
                 << endl;
        }

        // 等待1秒后发送下一个包
        sleep(1);
    }

    // 关闭套接字
    close(sock);

    return 0;
}
