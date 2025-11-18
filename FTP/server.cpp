/*
 * FTP 服务器端程序
 * 功能：接收客户端连接，处理 LIST、UPLOAD、DOWNLOAD、QUIT 命令
 * 支持多客户端并发连接（使用多线程）
 */

#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <sstream>

// Socket 相关头文件
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

using namespace std;

// 配置常量
const int PORT = 9999;                      // 服务器监听端口
const string SERVER_DIR = "server_files/"; // 服务器文件存储目录
const int BUFFER_SIZE = 4096;               // 缓冲区大小

// 网络字节序转换（64位）
uint64_t htonll(uint64_t value) {
    int num = 1;
    if (*(char*)&num == 1) { // 小端序
        uint32_t high = htonl((uint32_t)(value >> 32));
        uint32_t low = htonl((uint32_t)(value & 0xFFFFFFFF));
        return ((uint64_t)low << 32) | high;
    }
    return value; // 大端序
}

uint64_t ntohll(uint64_t value) {
    return htonll(value); // 转换是对称的
}

/**
 * 健壮的 send 函数
 * 确保发送指定长度的所有数据
 * @param sock 套接字
 * @param buffer 要发送的数据
 * @param length 数据长度
 * @return 成功返回 true，失败返回 false
 */
bool send_all(SOCKET sock, const char* buffer, size_t length) {
    size_t total_sent = 0;
    while (total_sent < length) {
        ssize_t sent = send(sock, buffer + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            cerr << "[ERROR] send 失败" << endl;
            return false;
        }
        total_sent += sent;
    }
    return true;
}

/**
 * 健壮的 recv 函数
 * 确保接收指定长度的所有数据
 * @param sock 套接字
 * @param buffer 接收缓冲区
 * @param length 期望接收的数据长度
 * @return 成功返回 true，失败返回 false
 */
bool recv_all(SOCKET sock, char* buffer, size_t length) {
    size_t total_received = 0;
    while (total_received < length) {
        ssize_t received = recv(sock, buffer + total_received, length - total_received, 0);
        if (received <= 0) {
            cerr << "[ERROR] recv 失败或连接关闭" << endl;
            return false;
        }
        total_received += received;
    }
    return true;
}

/**
 * 处理 LIST 命令
 * 列出服务器文件目录中的所有文件
 */
void handle_list(SOCKET client_sock) {
    cout << "[INFO] 处理 LIST 命令" << endl;

    DIR* dir = opendir(SERVER_DIR.c_str());
    if (!dir) {
        string error_msg = "ERROR cannot open directory\n";
        send_all(client_sock, error_msg.c_str(), error_msg.length());
        return;
    }

    // 遍历目录，收集文件名
    stringstream file_list;
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // 跳过 . 和 .. 目录
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 检查是否为普通文件
        string full_path = SERVER_DIR + entry->d_name;
        struct stat file_stat;
        if (stat(full_path.c_str(), &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            file_list << entry->d_name << "\n";
        }
    }
    closedir(dir);

    file_list << "END_OF_LIST\n";

    // 发送文件列表
    string list_str = file_list.str();
    send_all(client_sock, list_str.c_str(), list_str.length());

    cout << "[INFO] 文件列表已发送" << endl;
}

/**
 * 处理 UPLOAD 命令
 * 接收客户端上传的文件
 */
void handle_upload(SOCKET client_sock, const string& filename) {
    cout << "[INFO] 处理 UPLOAD 命令，文件名: " << filename << endl;

    // 检查文件名是否合法（简单检查，防止路径遍历攻击）
    if (filename.find("..") != string::npos || filename.find("/") != string::npos ||
        filename.find("\\") != string::npos) {
        string error_msg = "ERROR invalid filename\n";
        send_all(client_sock, error_msg.c_str(), error_msg.length());
        cout << "[ERROR] 非法文件名" << endl;
        return;
    }

    string full_path = SERVER_DIR + filename;

    // 检查文件是否已存在
    ifstream check_file(full_path);
    if (check_file.good()) {
        check_file.close();
        string error_msg = "ERROR file exists\n";
        send_all(client_sock, error_msg.c_str(), error_msg.length());
        cout << "[ERROR] 文件已存在" << endl;
        return;
    }

    // 发送 OK 响应
    string ok_msg = "OK\n";
    if (!send_all(client_sock, ok_msg.c_str(), ok_msg.length())) {
        return;
    }

    // 接收文件大小（64位，网络字节序）
    uint64_t file_size_net;
    if (!recv_all(client_sock, (char*)&file_size_net, sizeof(file_size_net))) {
        cout << "[ERROR] 接收文件大小失败" << endl;
        return;
    }
    uint64_t file_size = ntohll(file_size_net);
    cout << "[INFO] 文件大小: " << file_size << " 字节" << endl;

    // 打开文件准备写入
    ofstream out_file(full_path, ios::binary);
    if (!out_file) {
        cout << "[ERROR] 无法创建文件" << endl;
        return;
    }

    // 循环接收文件数据
    char buffer[BUFFER_SIZE];
    uint64_t total_received = 0;

    while (total_received < file_size) {
        size_t to_receive = min((uint64_t)BUFFER_SIZE, file_size - total_received);
        ssize_t received = recv(client_sock, buffer, to_receive, 0);

        if (received <= 0) {
            cout << "[ERROR] 接收文件数据失败" << endl;
            out_file.close();
            remove(full_path.c_str()); // 删除不完整的文件
            return;
        }

        out_file.write(buffer, received);
        total_received += received;

        // 显示进度
        if (total_received % (100 * BUFFER_SIZE) == 0 || total_received == file_size) {
            cout << "[PROGRESS] 已接收: " << total_received << "/" << file_size
                 << " (" << (total_received * 100 / file_size) << "%)" << endl;
        }
    }

    out_file.close();
    cout << "[SUCCESS] 文件上传成功: " << filename << endl;
}

/**
 * 处理 DOWNLOAD 命令
 * 发送文件给客户端
 */
void handle_download(SOCKET client_sock, const string& filename) {
    cout << "[INFO] 处理 DOWNLOAD 命令，文件名: " << filename << endl;

    // 检查文件名是否合法
    if (filename.find("..") != string::npos || filename.find("/") != string::npos ||
        filename.find("\\") != string::npos) {
        string error_msg = "ERROR invalid filename\n";
        send_all(client_sock, error_msg.c_str(), error_msg.length());
        cout << "[ERROR] 非法文件名" << endl;
        return;
    }

    string full_path = SERVER_DIR + filename;

    // 检查文件是否存在
    ifstream file(full_path, ios::binary | ios::ate);
    if (!file) {
        string error_msg = "ERROR not found\n";
        send_all(client_sock, error_msg.c_str(), error_msg.length());
        cout << "[ERROR] 文件不存在" << endl;
        return;
    }

    // 获取文件大小
    uint64_t file_size = file.tellg();
    file.seekg(0, ios::beg);

    // 发送 OK 响应
    string ok_msg = "OK\n";
    if (!send_all(client_sock, ok_msg.c_str(), ok_msg.length())) {
        file.close();
        return;
    }

    // 发送文件大小（网络字节序）
    uint64_t file_size_net = htonll(file_size);
    if (!send_all(client_sock, (char*)&file_size_net, sizeof(file_size_net))) {
        file.close();
        return;
    }

    cout << "[INFO] 文件大小: " << file_size << " 字节" << endl;

    // 循环发送文件数据
    char buffer[BUFFER_SIZE];
    uint64_t total_sent = 0;

    while (total_sent < file_size) {
        size_t to_read = min((uint64_t)BUFFER_SIZE, file_size - total_sent);
        file.read(buffer, to_read);
        size_t actually_read = file.gcount();

        if (actually_read == 0) {
            break;
        }

        if (!send_all(client_sock, buffer, actually_read)) {
            cout << "[ERROR] 发送文件数据失败" << endl;
            file.close();
            return;
        }

        total_sent += actually_read;

        // 显示进度
        if (total_sent % (100 * BUFFER_SIZE) == 0 || total_sent == file_size) {
            cout << "[PROGRESS] 已发送: " << total_sent << "/" << file_size
                 << " (" << (total_sent * 100 / file_size) << "%)" << endl;
        }
    }

    file.close();
    cout << "[SUCCESS] 文件下载完成: " << filename << endl;
}

/**
 * 客户端处理线程
 * 处理单个客户端的所有请求
 */
void handle_client(SOCKET client_sock, string client_addr) {
    cout << "[INFO] 客户端已连接: " << client_addr << endl;

    char buffer[BUFFER_SIZE];

    while (true) {
        // 接收命令（以换行符结束）
        memset(buffer, 0, BUFFER_SIZE);
        string command_line;

        // 逐字符接收，直到遇到换行符
        while (true) {
            char ch;
            ssize_t received = recv(client_sock, &ch, 1, 0);
            if (received <= 0) {
                cout << "[INFO] 客户端断开连接: " << client_addr << endl;
                close(client_sock);
                return;
            }

            if (ch == '\n') {
                break;
            }

            command_line += ch;
        }

        cout << "[INFO] 收到命令: " << command_line << endl;

        // 解析命令
        stringstream ss(command_line);
        string command;
        ss >> command;

        if (command == "LIST") {
            handle_list(client_sock);
        }
        else if (command == "UPLOAD") {
            string filename;
            ss >> filename;
            handle_upload(client_sock, filename);
        }
        else if (command == "DOWNLOAD") {
            string filename;
            ss >> filename;
            handle_download(client_sock, filename);
        }
        else if (command == "QUIT") {
            cout << "[INFO] 客户端退出: " << client_addr << endl;
            break;
        }
        else {
            string error_msg = "ERROR unknown command\n";
            send_all(client_sock, error_msg.c_str(), error_msg.length());
            cout << "[ERROR] 未知命令: " << command << endl;
        }
    }

    close(client_sock);
    cout << "[INFO] 客户端连接关闭: " << client_addr << endl;
}

int main() {
    cout << "========================================" << endl;
    cout << "       简易 FTP 服务器启动中...        " << endl;
    cout << "========================================" << endl;

#ifdef _WIN32
    // Windows 需要初始化 Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        cerr << "[FATAL] WSAStartup 失败" << endl;
        return 1;
    }
#endif

    // 创建服务器文件目录
    struct stat st;
    if (stat(SERVER_DIR.c_str(), &st) != 0) {
        #ifdef _WIN32
            mkdir(SERVER_DIR.c_str());
        #else
            mkdir(SERVER_DIR.c_str(), 0755);
        #endif
        cout << "[INFO] 已创建服务器文件目录: " << SERVER_DIR << endl;
    }

    // 1. 创建 Socket
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock == INVALID_SOCKET) {
        cerr << "[FATAL] 创建 socket 失败" << endl;
        return 1;
    }

    // 设置 SO_REUSEADDR 选项，允许端口重用
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // 2. 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 监听所有网络接口
    server_addr.sin_port = htons(PORT);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "[FATAL] bind 失败，端口可能被占用" << endl;
        close(server_sock);
        return 1;
    }

    // 3. 开始监听
    if (listen(server_sock, 10) == SOCKET_ERROR) {
        cerr << "[FATAL] listen 失败" << endl;
        close(server_sock);
        return 1;
    }

    cout << "[SUCCESS] 服务器启动成功，监听端口: " << PORT << endl;
    cout << "[INFO] 文件存储目录: " << SERVER_DIR << endl;
    cout << "[INFO] 等待客户端连接..." << endl;
    cout << "========================================" << endl;

    // 4. 主循环：接受客户端连接
    vector<thread> client_threads;

    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        // 接受新的客户端连接
        SOCKET client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock == INVALID_SOCKET) {
            cerr << "[ERROR] accept 失败" << endl;
            continue;
        }

        // 获取客户端地址信息
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        string client_info = string(client_ip) + ":" + to_string(ntohs(client_addr.sin_port));

        // 为每个客户端创建新线程处理
        thread client_thread(handle_client, client_sock, client_info);
        client_thread.detach(); // 分离线程，让其独立运行
    }

    close(server_sock);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
