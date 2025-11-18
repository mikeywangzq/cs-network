/*
 * FTP 客户端程序
 * 功能：连接到服务器，支持 ls、upload、download、quit 命令
 * 提供简单的命令行交互界面
 */

#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

// Socket 相关头文件
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
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
const string SERVER_IP = "127.0.0.1";  // 服务器 IP 地址
const int SERVER_PORT = 9999;          // 服务器端口
const int BUFFER_SIZE = 4096;          // 缓冲区大小

// 全局 socket
SOCKET client_sock = INVALID_SOCKET;

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
 * 接收服务器响应（以换行符结束的文本）
 * @return 响应字符串
 */
string recv_response() {
    string response;
    char ch;

    while (true) {
        ssize_t received = recv(client_sock, &ch, 1, 0);
        if (received <= 0) {
            cerr << "[ERROR] 接收响应失败" << endl;
            return "";
        }

        if (ch == '\n') {
            break;
        }

        response += ch;
    }

    return response;
}

/**
 * 处理 ls 命令
 * 列出服务器上的文件
 */
void cmd_list() {
    cout << "[INFO] 发送 LIST 命令..." << endl;

    // 发送 LIST 命令
    string command = "LIST\n";
    if (!send_all(client_sock, command.c_str(), command.length())) {
        return;
    }

    // 接收文件列表
    cout << "\n========== 服务器文件列表 ==========" << endl;

    while (true) {
        string line = recv_response();
        if (line.empty()) {
            break;
        }

        if (line == "END_OF_LIST") {
            break;
        }

        if (line.substr(0, 5) == "ERROR") {
            cout << "[ERROR] " << line << endl;
            break;
        }

        cout << "  " << line << endl;
    }

    cout << "====================================" << endl << endl;
}

/**
 * 处理 upload 命令
 * 上传本地文件到服务器
 */
void cmd_upload(const string& filename) {
    // 检查文件是否存在
    ifstream file(filename, ios::binary | ios::ate);
    if (!file) {
        cout << "[ERROR] 本地文件不存在: " << filename << endl;
        return;
    }

    // 获取文件大小
    uint64_t file_size = file.tellg();
    file.seekg(0, ios::beg);

    cout << "[INFO] 准备上传文件: " << filename << " (大小: " << file_size << " 字节)" << endl;

    // 发送 UPLOAD 命令
    string command = "UPLOAD " + filename + "\n";
    if (!send_all(client_sock, command.c_str(), command.length())) {
        file.close();
        return;
    }

    // 接收服务器响应
    string response = recv_response();
    cout << "[SERVER] " << response << endl;

    if (response.substr(0, 5) == "ERROR") {
        file.close();
        return;
    }

    if (response != "OK") {
        cout << "[ERROR] 意外的服务器响应" << endl;
        file.close();
        return;
    }

    // 发送文件大小（网络字节序）
    uint64_t file_size_net = htonll(file_size);
    if (!send_all(client_sock, (char*)&file_size_net, sizeof(file_size_net))) {
        file.close();
        return;
    }

    // 循环发送文件数据
    char buffer[BUFFER_SIZE];
    uint64_t total_sent = 0;

    cout << "[INFO] 开始上传文件..." << endl;

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
            cout << "[PROGRESS] 已上传: " << total_sent << "/" << file_size
                 << " (" << (total_sent * 100 / file_size) << "%)" << endl;
        }
    }

    file.close();
    cout << "[SUCCESS] 文件上传成功！" << endl << endl;
}

/**
 * 处理 download 命令
 * 从服务器下载文件
 */
void cmd_download(const string& filename) {
    cout << "[INFO] 准备下载文件: " << filename << endl;

    // 发送 DOWNLOAD 命令
    string command = "DOWNLOAD " + filename + "\n";
    if (!send_all(client_sock, command.c_str(), command.length())) {
        return;
    }

    // 接收服务器响应
    string response = recv_response();
    cout << "[SERVER] " << response << endl;

    if (response.substr(0, 5) == "ERROR") {
        return;
    }

    if (response != "OK") {
        cout << "[ERROR] 意外的服务器响应" << endl;
        return;
    }

    // 接收文件大小
    uint64_t file_size_net;
    if (!recv_all(client_sock, (char*)&file_size_net, sizeof(file_size_net))) {
        cout << "[ERROR] 接收文件大小失败" << endl;
        return;
    }
    uint64_t file_size = ntohll(file_size_net);
    cout << "[INFO] 文件大小: " << file_size << " 字节" << endl;

    // 打开文件准备写入
    string local_filename = "client_files/" + filename;
    ofstream out_file(local_filename, ios::binary);
    if (!out_file) {
        cout << "[ERROR] 无法创建本地文件: " << local_filename << endl;
        return;
    }

    // 循环接收文件数据
    char buffer[BUFFER_SIZE];
    uint64_t total_received = 0;

    cout << "[INFO] 开始下载文件..." << endl;

    while (total_received < file_size) {
        size_t to_receive = min((uint64_t)BUFFER_SIZE, file_size - total_received);
        ssize_t received = recv(client_sock, buffer, to_receive, 0);

        if (received <= 0) {
            cout << "[ERROR] 接收文件数据失败" << endl;
            out_file.close();
            remove(local_filename.c_str()); // 删除不完整的文件
            return;
        }

        out_file.write(buffer, received);
        total_received += received;

        // 显示进度
        if (total_received % (100 * BUFFER_SIZE) == 0 || total_received == file_size) {
            cout << "[PROGRESS] 已下载: " << total_received << "/" << file_size
                 << " (" << (total_received * 100 / file_size) << "%)" << endl;
        }
    }

    out_file.close();
    cout << "[SUCCESS] 文件下载成功，保存至: " << local_filename << endl << endl;
}

/**
 * 显示帮助信息
 */
void show_help() {
    cout << "\n========== 可用命令 ==========" << endl;
    cout << "  ls                  - 列出服务器上的文件" << endl;
    cout << "  upload <filename>   - 上传文件到服务器" << endl;
    cout << "  download <filename> - 从服务器下载文件" << endl;
    cout << "  help                - 显示此帮助信息" << endl;
    cout << "  quit                - 退出程序" << endl;
    cout << "===============================" << endl << endl;
}

/**
 * 连接到服务器
 */
bool connect_to_server() {
    cout << "[INFO] 连接到服务器 " << SERVER_IP << ":" << SERVER_PORT << "..." << endl;

    // 创建 Socket
    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock == INVALID_SOCKET) {
        cerr << "[FATAL] 创建 socket 失败" << endl;
        return false;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);

    // 将 IP 地址从字符串转换为二进制
    if (inet_pton(AF_INET, SERVER_IP.c_str(), &server_addr.sin_addr) <= 0) {
        cerr << "[FATAL] 无效的服务器 IP 地址" << endl;
        close(client_sock);
        return false;
    }

    // 连接到服务器
    if (connect(client_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "[FATAL] 连接服务器失败，请确保服务器已启动" << endl;
        close(client_sock);
        return false;
    }

    cout << "[SUCCESS] 已连接到服务器！" << endl << endl;
    return true;
}

int main() {
    cout << "========================================" << endl;
    cout << "       简易 FTP 客户端启动中...        " << endl;
    cout << "========================================" << endl << endl;

#ifdef _WIN32
    // Windows 需要初始化 Winsock
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        cerr << "[FATAL] WSAStartup 失败" << endl;
        return 1;
    }
#endif

    // 创建客户端文件目录
    struct stat st;
    if (stat("client_files", &st) != 0) {
        #ifdef _WIN32
            mkdir("client_files");
        #else
            mkdir("client_files", 0755);
        #endif
        cout << "[INFO] 已创建客户端文件目录: client_files/" << endl;
    }

    // 连接到服务器
    if (!connect_to_server()) {
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    show_help();

    // 主循环：处理用户命令
    while (true) {
        cout << "ftp> ";
        string input;
        getline(cin, input);

        // 去除前后空白
        size_t start = input.find_first_not_of(" \t");
        size_t end = input.find_last_not_of(" \t");
        if (start == string::npos) {
            continue; // 空行
        }
        input = input.substr(start, end - start + 1);

        if (input.empty()) {
            continue;
        }

        // 解析命令
        stringstream ss(input);
        string command;
        ss >> command;

        if (command == "ls" || command == "list") {
            cmd_list();
        }
        else if (command == "upload") {
            string filename;
            ss >> filename;
            if (filename.empty()) {
                cout << "[ERROR] 用法: upload <filename>" << endl;
            } else {
                cmd_upload(filename);
            }
        }
        else if (command == "download") {
            string filename;
            ss >> filename;
            if (filename.empty()) {
                cout << "[ERROR] 用法: download <filename>" << endl;
            } else {
                cmd_download(filename);
            }
        }
        else if (command == "help") {
            show_help();
        }
        else if (command == "quit" || command == "exit") {
            cout << "[INFO] 发送退出命令..." << endl;
            string quit_cmd = "QUIT\n";
            send_all(client_sock, quit_cmd.c_str(), quit_cmd.length());
            break;
        }
        else {
            cout << "[ERROR] 未知命令: " << command << endl;
            cout << "输入 'help' 查看可用命令" << endl;
        }
    }

    // 关闭连接
    close(client_sock);
    cout << "[INFO] 已断开连接，程序退出。" << endl;

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
