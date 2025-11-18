/*
 * ============================================================================
 * 文件名: client.cpp
 * 描述: 聊天室客户端（双线程模型）
 * 架构: 主线程发送消息 + 接收线程接收消息
 * 平台: Linux / macOS
 * ============================================================================
 */

#include <iostream>
#include <cstring>
#include <thread>
#include <atomic>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// 配置常量
const int BUFFER_SIZE = 4096;

// 全局变量
std::atomic<bool> g_running(true);  // 程序运行标志

/*
 * ============================================================================
 * 函数名: receive_thread
 * 功能: 接收线程函数，持续接收服务器消息并显示
 * 参数: sock_fd - 套接字文件描述符
 * ============================================================================
 */
void receive_thread(int sock_fd) {
    char buffer[BUFFER_SIZE];

    while (g_running) {
        memset(buffer, 0, BUFFER_SIZE);

        // 接收服务器消息
        ssize_t bytes_received = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            // 成功接收消息，打印到控制台
            std::cout << buffer << std::flush;
        }
        else if (bytes_received == 0) {
            // 服务器关闭连接
            std::cout << "\n[系统] 服务器已断开连接" << std::endl;
            g_running = false;
            break;
        }
        else {
            // 接收错误
            if (g_running) {  // 只有在运行状态才报错
                std::cerr << "[错误] 接收消息失败: " << strerror(errno) << std::endl;
                g_running = false;
            }
            break;
        }
    }
}

/*
 * ============================================================================
 * 主函数
 * ============================================================================
 */
int main(int argc, char* argv[]) {
    // 解析命令行参数
    const char* server_ip = "127.0.0.1";  // 默认服务器 IP
    int server_port = 8888;               // 默认服务器端口

    if (argc >= 2) {
        server_ip = argv[1];
    }
    if (argc >= 3) {
        server_port = atoi(argv[2]);
    }

    std::cout << R"(
╔════════════════════════════════════════╗
║         聊天室客户端 v1.0            ║
╚════════════════════════════════════════╝
)" << std::endl;

    // ========================================================================
    // 1. 创建套接字
    // ========================================================================
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        std::cerr << "[错误] socket 创建失败: " << strerror(errno) << std::endl;
        return 1;
    }

    // ========================================================================
    // 2. 连接到服务器
    // ========================================================================
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        std::cerr << "[错误] 无效的 IP 地址: " << server_ip << std::endl;
        close(sock_fd);
        return 1;
    }

    std::cout << "[连接] 正在连接到服务器 " << server_ip << ":" << server_port << "..." << std::endl;

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "[错误] 连接服务器失败: " << strerror(errno) << std::endl;
        close(sock_fd);
        return 1;
    }

    std::cout << "[成功] 已连接到服务器\n" << std::endl;

    // ========================================================================
    // 3. 启动接收线程
    // ========================================================================
    std::thread recv_thread(receive_thread, sock_fd);

    // ========================================================================
    // 4. 主线程：读取用户输入并发送到服务器
    // ========================================================================
    std::string input;

    while (g_running) {
        // 读取用户输入
        if (!std::getline(std::cin, input)) {
            // EOF 或读取错误
            break;
        }

        // 检查是否退出命令
        if (input == "/quit" || input == "/exit") {
            std::cout << "[系统] 正在退出..." << std::endl;
            break;
        }

        // 忽略空行
        if (input.empty()) {
            continue;
        }

        // 添加换行符
        input += "\n";

        // 发送消息到服务器
        ssize_t bytes_sent = send(sock_fd, input.c_str(), input.length(), 0);

        if (bytes_sent == -1) {
            std::cerr << "[错误] 发送消息失败: " << strerror(errno) << std::endl;
            break;
        }
    }

    // ========================================================================
    // 5. 清理资源
    // ========================================================================
    g_running = false;

    // 关闭套接字（这会让接收线程退出）
    shutdown(sock_fd, SHUT_RDWR);
    close(sock_fd);

    // 等待接收线程结束
    if (recv_thread.joinable()) {
        recv_thread.join();
    }

    std::cout << "\n客户端已退出" << std::endl;
    return 0;
}
