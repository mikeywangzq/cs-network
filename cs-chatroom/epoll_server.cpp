/*
 * ============================================================================
 * 文件名: epoll_server.cpp
 * 描述: 基于 epoll 的高性能多人聊天室服务器
 * 架构: 单线程 + I/O 多路复用 (epoll)
 * 平台: 仅限 Linux
 * ============================================================================
 */

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <map>
#include <string>
#include <ctime>

// 配置常量
const int PORT = 8888;              // 服务器监听端口
const int MAX_EVENTS = 100;         // epoll_wait 一次最多返回的事件数
const int BUFFER_SIZE = 4096;       // 接收缓冲区大小
const int MAX_CLIENTS = 1000;       // 最大客户端连接数

// 客户端信息结构体
struct ClientInfo {
    int sock_fd;                    // 客户端套接字文件描述符
    std::string nickname;           // 客户端昵称
    std::string ip;                 // 客户端 IP 地址
    int port;                       // 客户端端口
    time_t connect_time;            // 连接时间
};

// 全局变量：客户端映射表 (fd -> ClientInfo)
std::map<int, ClientInfo> g_clients;

/*
 * ============================================================================
 * 函数名: set_nonblocking
 * 功能: 将文件描述符设置为非阻塞模式 (关键点 1)
 * 参数: fd - 文件描述符
 * 返回值: true 成功, false 失败
 * 说明: 非阻塞模式下，read/write/accept 等操作不会阻塞，
 *       如果没有数据可读/写，会立即返回 EWOULDBLOCK 或 EAGAIN
 * ============================================================================
 */
bool set_nonblocking(int fd) {
    // 获取当前文件描述符的标志
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "[错误] fcntl F_GETFL 失败: " << strerror(errno) << std::endl;
        return false;
    }

    // 添加 O_NONBLOCK 标志，设置为非阻塞
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "[错误] fcntl F_SETFL O_NONBLOCK 失败: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

/*
 * ============================================================================
 * 函数名: create_listen_socket
 * 功能: 创建并初始化监听套接字
 * 返回值: 监听套接字的文件描述符，失败返回 -1
 * ============================================================================
 */
int create_listen_socket() {
    // 1. 创建套接字
    int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == -1) {
        std::cerr << "[错误] socket 创建失败: " << strerror(errno) << std::endl;
        return -1;
    }

    // 2. 设置套接字选项：地址重用（避免 TIME_WAIT 状态导致的端口占用）
    int opt = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        std::cerr << "[警告] setsockopt SO_REUSEADDR 失败: " << strerror(errno) << std::endl;
    }

    // 3. 【关键】将监听套接字设置为非阻塞模式
    if (!set_nonblocking(listen_sock)) {
        close(listen_sock);
        return -1;
    }

    // 4. 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // 监听所有网络接口
    server_addr.sin_port = htons(PORT);

    if (bind(listen_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "[错误] bind 失败: " << strerror(errno) << std::endl;
        close(listen_sock);
        return -1;
    }

    // 5. 开始监听
    if (listen(listen_sock, SOMAXCONN) == -1) {
        std::cerr << "[错误] listen 失败: " << strerror(errno) << std::endl;
        close(listen_sock);
        return -1;
    }

    std::cout << "[成功] 服务器启动，监听端口: " << PORT << std::endl;
    return listen_sock;
}

/*
 * ============================================================================
 * 函数名: broadcast_message
 * 功能: 广播消息给所有客户端（除了发送者自己）
 * 参数:
 *   sender_fd - 发送者的文件描述符（-1 表示系统消息，发给所有人）
 *   message - 要广播的消息
 * 说明: 非阻塞 send 可能返回 EWOULDBLOCK，这里简化处理，实际生产环境
 *       应该使用发送缓冲队列处理
 * ============================================================================
 */
void broadcast_message(int sender_fd, const std::string& message) {
    // 遍历所有连接的客户端
    for (auto& pair : g_clients) {
        int client_fd = pair.first;

        // 不发送给自己
        if (client_fd == sender_fd) {
            continue;
        }

        // 非阻塞发送
        ssize_t sent = send(client_fd, message.c_str(), message.length(), MSG_NOSIGNAL);

        if (sent == -1) {
            // EWOULDBLOCK/EAGAIN: 发送缓冲区满，数据暂时无法发送
            // 生产环境应该将数据加入发送队列，等待 EPOLLOUT 事件再发送
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::cerr << "[警告] 发送缓冲区满，客户端 fd=" << client_fd
                          << " 消息丢失" << std::endl;
            } else {
                std::cerr << "[错误] 发送失败 fd=" << client_fd
                          << ": " << strerror(errno) << std::endl;
            }
        }
    }
}

/*
 * ============================================================================
 * 函数名: handle_new_connection
 * 功能: 处理新的客户端连接 (Case 1)
 * 参数:
 *   listen_sock - 监听套接字
 *   epoll_fd - epoll 实例的文件描述符
 * 说明:
 *   1. 使用 accept 接受新连接
 *   2. 将新连接设置为非阻塞
 *   3. 将新连接添加到 epoll 实例中，监听 EPOLLIN | EPOLLET
 * ============================================================================
 */
void handle_new_connection(int listen_sock, int epoll_fd) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // 循环 accept，因为边缘触发模式下可能有多个连接等待
    while (true) {
        int client_sock = accept(listen_sock, (struct sockaddr*)&client_addr, &addr_len);

        if (client_sock == -1) {
            // EWOULDBLOCK/EAGAIN: 没有更多连接了（正常情况）
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                break;  // 所有连接都处理完了
            }
            // 其他错误
            std::cerr << "[错误] accept 失败: " << strerror(errno) << std::endl;
            break;
        }

        // 检查客户端数量限制
        if (g_clients.size() >= MAX_CLIENTS) {
            std::cerr << "[警告] 客户端数量已达上限，拒绝连接" << std::endl;
            const char* msg = "服务器已满，请稍后再试\n";
            send(client_sock, msg, strlen(msg), 0);
            close(client_sock);
            continue;
        }

        // 【关键】将客户端套接字设置为非阻塞
        if (!set_nonblocking(client_sock)) {
            close(client_sock);
            continue;
        }

        // 准备 epoll 事件
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;  // 监听可读事件 + 边缘触发模式
        ev.data.fd = client_sock;

        // 【关键】使用 epoll_ctl 的 EPOLL_CTL_ADD 将客户端套接字添加到 epoll 实例
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_sock, &ev) == -1) {
            std::cerr << "[错误] epoll_ctl EPOLL_CTL_ADD 失败: "
                      << strerror(errno) << std::endl;
            close(client_sock);
            continue;
        }

        // 获取客户端 IP 和端口
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        // 创建客户端信息
        ClientInfo client_info;
        client_info.sock_fd = client_sock;
        client_info.nickname = "用户" + std::to_string(client_sock);  // 默认昵称
        client_info.ip = client_ip;
        client_info.port = client_port;
        client_info.connect_time = time(nullptr);

        // 添加到客户端列表
        g_clients[client_sock] = client_info;

        std::cout << "[连接] 新客户端 fd=" << client_sock
                  << " (" << client_ip << ":" << client_port << ")"
                  << " 当前在线: " << g_clients.size() << std::endl;

        // 向新客户端发送欢迎消息
        std::string welcome = "=== 欢迎来到聊天室 ===\n"
                             "当前在线人数: " + std::to_string(g_clients.size()) + "\n"
                             "输入消息即可发送\n"
                             "====================\n";
        send(client_sock, welcome.c_str(), welcome.length(), MSG_NOSIGNAL);

        // 广播新用户加入消息
        std::string join_msg = "[系统] " + client_info.nickname + " 加入了聊天室\n";
        broadcast_message(client_sock, join_msg);
    }
}

/*
 * ============================================================================
 * 函数名: handle_client_message
 * 功能: 处理客户端发来的消息 (Case 2)
 * 参数:
 *   client_sock - 客户端套接字
 *   epoll_fd - epoll 实例的文件描述符
 * 返回值: true 继续保持连接, false 需要关闭连接
 * 说明:
 *   1. 非阻塞 recv，循环读取直到 EWOULDBLOCK
 *   2. 处理客户端断开（recv 返回 0 或错误）
 *   3. 广播消息给其他客户端
 * ============================================================================
 */
bool handle_client_message(int client_sock, int epoll_fd) {
    char buffer[BUFFER_SIZE];
    std::string full_message;

    // 【关键】边缘触发模式下，必须循环 recv 直到 EWOULDBLOCK
    // 因为边缘触发只在状态变化时通知一次
    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = recv(client_sock, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_read > 0) {
            // 成功读取数据
            full_message.append(buffer, bytes_read);

            // 继续读取，直到读完所有数据
        }
        else if (bytes_read == 0) {
            // 客户端正常关闭连接
            std::cout << "[断开] 客户端 fd=" << client_sock
                      << " 正常断开连接" << std::endl;
            return false;  // 需要关闭连接
        }
        else {  // bytes_read == -1
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                // 【正常情况】没有更多数据可读了
                break;
            }
            else if (errno == EINTR) {
                // 被信号中断，继续读取
                continue;
            }
            else {
                // 其他错误，连接异常
                std::cerr << "[错误] recv 失败 fd=" << client_sock
                          << ": " << strerror(errno) << std::endl;
                return false;  // 需要关闭连接
            }
        }
    }

    // 如果读取到了数据，进行处理
    if (!full_message.empty()) {
        auto it = g_clients.find(client_sock);
        if (it != g_clients.end()) {
            // 格式化消息: [昵称] 消息内容
            std::string formatted_msg = "[" + it->second.nickname + "] " + full_message;

            std::cout << "[消息] fd=" << client_sock << " " << formatted_msg;

            // 广播消息给所有其他客户端
            broadcast_message(client_sock, formatted_msg);
        }
    }

    return true;  // 保持连接
}

/*
 * ============================================================================
 * 函数名: close_client_connection
 * 功能: 关闭客户端连接并清理资源
 * 参数:
 *   client_sock - 客户端套接字
 *   epoll_fd - epoll 实例的文件描述符
 * 说明:
 *   1. 使用 epoll_ctl 的 EPOLL_CTL_DEL 从 epoll 实例中移除
 *   2. 关闭套接字
 *   3. 从客户端列表中删除
 *   4. 广播用户离开消息
 * ============================================================================
 */
void close_client_connection(int client_sock, int epoll_fd) {
    auto it = g_clients.find(client_sock);
    if (it == g_clients.end()) {
        return;  // 客户端不存在
    }

    std::string nickname = it->second.nickname;

    // 【关键】使用 epoll_ctl 的 EPOLL_CTL_DEL 将客户端从 epoll 实例中移除
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_sock, nullptr) == -1) {
        std::cerr << "[警告] epoll_ctl EPOLL_CTL_DEL 失败 fd=" << client_sock
                  << ": " << strerror(errno) << std::endl;
    }

    // 关闭套接字
    close(client_sock);

    // 从客户端列表中删除
    g_clients.erase(it);

    std::cout << "[离线] " << nickname << " fd=" << client_sock
              << " 已断开，当前在线: " << g_clients.size() << std::endl;

    // 广播用户离开消息
    std::string leave_msg = "[系统] " + nickname + " 离开了聊天室\n";
    broadcast_message(-1, leave_msg);  // -1 表示发送给所有人
}

/*
 * ============================================================================
 * 主函数：事件循环 (Event Loop)
 * ============================================================================
 */
int main() {
    std::cout << R"(
╔════════════════════════════════════════╗
║   基于 Epoll 的高性能聊天室服务器    ║
║   架构: 单线程 + I/O 多路复用        ║
║   作者: C++ 服务器架构师             ║
╚════════════════════════════════════════╝
)" << std::endl;

    // ========================================================================
    // 1. 创建监听套接字（已设置为非阻塞）
    // ========================================================================
    int listen_sock = create_listen_socket();
    if (listen_sock == -1) {
        return 1;
    }

    // ========================================================================
    // 2. 创建 epoll 实例
    // ========================================================================
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "[错误] epoll_create1 失败: " << strerror(errno) << std::endl;
        close(listen_sock);
        return 1;
    }
    std::cout << "[成功] epoll 实例创建成功 fd=" << epoll_fd << std::endl;

    // ========================================================================
    // 3. 将监听套接字添加到 epoll 实例
    // ========================================================================
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 监听可读事件 + 边缘触发
    ev.data.fd = listen_sock;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        std::cerr << "[错误] epoll_ctl EPOLL_CTL_ADD listen_sock 失败: "
                  << strerror(errno) << std::endl;
        close(epoll_fd);
        close(listen_sock);
        return 1;
    }
    std::cout << "[成功] 监听套接字已添加到 epoll 实例" << std::endl;

    // ========================================================================
    // 4. 【关键点 2】主事件循环 (Event Loop)
    // ========================================================================
    struct epoll_event events[MAX_EVENTS];

    std::cout << "\n服务器运行中，等待客户端连接...\n" << std::endl;

    while (true) {
        // 等待事件发生（阻塞调用，-1 表示无限等待）
        // 返回值：就绪的文件描述符数量
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        if (nfds == -1) {
            if (errno == EINTR) {
                // 被信号中断，继续循环
                continue;
            }
            std::cerr << "[错误] epoll_wait 失败: " << strerror(errno) << std::endl;
            break;
        }

        // 遍历所有就绪的事件
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;

            // ================================================================
            // Case 1: 监听套接字有事件 -> 有新连接
            // ================================================================
            if (fd == listen_sock) {
                handle_new_connection(listen_sock, epoll_fd);
            }
            // ================================================================
            // Case 2: 客户端套接字有事件 -> 客户端发来数据
            // ================================================================
            else {
                // 检查是否有错误事件
                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    std::cerr << "[错误] 客户端 fd=" << fd
                              << " 发生错误事件，关闭连接" << std::endl;
                    close_client_connection(fd, epoll_fd);
                    continue;
                }

                // 处理客户端消息
                if (events[i].events & EPOLLIN) {
                    bool keep_alive = handle_client_message(fd, epoll_fd);
                    if (!keep_alive) {
                        // 客户端断开或发生错误，关闭连接
                        close_client_connection(fd, epoll_fd);
                    }
                }
            }
        }
    }

    // ========================================================================
    // 5. 清理资源
    // ========================================================================
    std::cout << "\n服务器关闭中..." << std::endl;

    // 关闭所有客户端连接
    for (auto& pair : g_clients) {
        close(pair.first);
    }
    g_clients.clear();

    // 关闭 epoll 和监听套接字
    close(epoll_fd);
    close(listen_sock);

    std::cout << "服务器已关闭" << std::endl;
    return 0;
}
