/**
 * POP3 客户端 - 极简学习版本
 *
 * 功能：连接到 POP3 服务器（明文端口 110），登录，列出邮件，下载邮件
 * 协议：Post Office Protocol - Version 3 (RFC 1939)
 * 警告：此程序使用明文连接（端口 110），不支持 SSL/TLS
 *       现代邮件服务器通常要求 SSL/TLS（端口 995），此代码仅用于学习
 *
 * 编译方法：
 *   Linux/macOS: g++ -o pop3_client pop3_client.cpp
 *   Windows: cl pop3_client.cpp /link ws2_32.lib
 */

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>

// 平台相关的头文件
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

using namespace std;

// ============================================================================
// 配置区域 - 请修改为你的实际配置
// ============================================================================
const string POP3_SERVER = "pop.163.com";     // POP3 服务器地址
const int POP3_PORT = 110;                     // 明文端口（不加密）
const string USERNAME = "your_email@163.com";  // 你的邮箱用户名
const string PASSWORD = "your_password";       // 你的邮箱密码或授权码

// ============================================================================
// 工具函数
// ============================================================================

/**
 * 跨平台 Socket 初始化
 */
bool initSocket() {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "[错误] WSAStartup 失败" << endl;
        return false;
    }
#endif
    return true;
}

/**
 * 跨平台 Socket 清理
 */
void cleanupSocket() {
#ifdef _WIN32
    WSACleanup();
#endif
}

/**
 * 发送命令到 POP3 服务器
 * POP3 协议要求每条命令以 \r\n 结尾
 */
bool sendCommand(SOCKET sock, const string& command) {
    string cmd = command + "\r\n";
    int sent = send(sock, cmd.c_str(), cmd.length(), 0);
    if (sent == SOCKET_ERROR) {
        cerr << "[错误] 发送命令失败: " << command << endl;
        return false;
    }
    cout << "[发送] " << command << endl;
    return true;
}

/**
 * 接收服务器的单行响应
 * POP3 协议：成功响应以 +OK 开头，错误响应以 -ERR 开头
 */
string receiveResponse(SOCKET sock) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        return "";
    }

    string response(buffer, received);
    cout << "[接收] " << response;
    return response;
}

/**
 * 接收多行响应（用于 LIST 和 RETR 命令）
 * POP3 协议：多行响应以单独一行的 ".\r\n" 作为结束标记
 */
string receiveMultilineResponse(SOCKET sock) {
    string fullResponse;
    char buffer[4096];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int received = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (received <= 0) {
            break;
        }

        fullResponse.append(buffer, received);

        // 检查是否收到结束标记 ".\r\n"
        // 注意：根据 RFC 1939，结束标记是单独一行的点号
        size_t pos = fullResponse.find("\r\n.\r\n");
        if (pos != string::npos) {
            // 找到结束标记，截取到这里为止
            fullResponse = fullResponse.substr(0, pos + 2); // 保留第一个 \r\n
            break;
        }
    }

    return fullResponse;
}

/**
 * 检查响应是否成功（以 +OK 开头）
 */
bool isOK(const string& response) {
    return response.find("+OK") == 0;
}

/**
 * 解析 STAT 命令的响应，提取邮件数量
 * 格式: +OK <message_count> <total_size>
 */
int parseMessageCount(const string& response) {
    istringstream iss(response);
    string ok;
    int count;
    iss >> ok >> count;
    return count;
}

// ============================================================================
// POP3 客户端主逻辑
// ============================================================================

int main() {
    cout << "========================================" << endl;
    cout << "     POP3 客户端 - 学习演示版本" << endl;
    cout << "========================================" << endl;
    cout << endl;

    // Step 0: 初始化 Socket 库
    if (!initSocket()) {
        return 1;
    }

    // Step 1: 创建 TCP Socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        cerr << "[错误] 创建 Socket 失败" << endl;
        cleanupSocket();
        return 1;
    }
    cout << "[信息] Socket 创建成功" << endl;

    // Step 2: 解析服务器地址
    struct hostent* host = gethostbyname(POP3_SERVER.c_str());
    if (host == NULL) {
        cerr << "[错误] 无法解析主机名: " << POP3_SERVER << endl;
        closesocket(sock);
        cleanupSocket();
        return 1;
    }

    // Step 3: 设置服务器地址结构
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(POP3_PORT);
    memcpy(&serverAddr.sin_addr, host->h_addr_list[0], host->h_length);

    cout << "[信息] 正在连接到 " << POP3_SERVER << ":" << POP3_PORT << " ..." << endl;

    // Step 4: 连接到服务器
    if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "[错误] 连接失败" << endl;
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    cout << "[信息] 连接成功！" << endl;
    cout << endl;

    // ========================================================================
    // POP3 状态机：连接后立即进入 AUTHORIZATION 状态
    // ========================================================================

    // Step 5: 接收服务器欢迎消息
    cout << "--- 接收服务器欢迎消息 ---" << endl;
    string welcome = receiveResponse(sock);
    if (!isOK(welcome)) {
        cerr << "[错误] 服务器欢迎消息异常" << endl;
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    cout << endl;

    // ========================================================================
    // AUTHORIZATION 状态：使用 USER 和 PASS 命令进行身份验证
    // ========================================================================

    cout << "--- 开始身份验证 (AUTHORIZATION 状态) ---" << endl;

    // Step 6: 发送 USER 命令
    if (!sendCommand(sock, "USER " + USERNAME)) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    string userResponse = receiveResponse(sock);
    if (!isOK(userResponse)) {
        cerr << "[错误] USER 命令失败" << endl;
        closesocket(sock);
        cleanupSocket();
        return 1;
    }

    // Step 7: 发送 PASS 命令
    if (!sendCommand(sock, "PASS " + PASSWORD)) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    string passResponse = receiveResponse(sock);
    if (!isOK(passResponse)) {
        cerr << "[错误] PASS 命令失败，登录失败！" << endl;
        cerr << "[提示] 请检查用户名和密码是否正确" << endl;
        cerr << "[提示] 如果使用 163/QQ 等邮箱，可能需要使用授权码而不是密码" << endl;
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    cout << "[信息] 登录成功！邮箱已锁定" << endl;
    cout << endl;

    // ========================================================================
    // TRANSACTION 状态：邮箱已锁定，可以执行邮件操作
    // ========================================================================

    cout << "--- 进入事务状态 (TRANSACTION 状态) ---" << endl;

    // Step 8: 发送 STAT 命令 - 获取邮箱状态
    cout << "\n[1] 获取邮箱状态 (STAT 命令)" << endl;
    if (!sendCommand(sock, "STAT")) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    string statResponse = receiveResponse(sock);
    if (!isOK(statResponse)) {
        cerr << "[错误] STAT 命令失败" << endl;
    } else {
        int messageCount = parseMessageCount(statResponse);
        cout << "[信息] 邮箱中共有 " << messageCount << " 封邮件" << endl;
    }

    // Step 9: 发送 LIST 命令 - 列出所有邮件
    cout << "\n[2] 列出所有邮件 (LIST 命令)" << endl;
    if (!sendCommand(sock, "LIST")) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }

    // 先接收第一行 +OK 响应
    string listFirstLine = receiveResponse(sock);
    if (!isOK(listFirstLine)) {
        cerr << "[错误] LIST 命令失败" << endl;
    } else {
        // 接收多行响应（邮件列表）
        cout << "[信息] 邮件列表：" << endl;
        string listResponse = receiveMultilineResponse(sock);
        cout << listResponse << endl;
    }

    // Step 10: 发送 RETR 命令 - 下载第一封邮件
    cout << "\n[3] 下载第一封邮件 (RETR 1 命令)" << endl;
    if (!sendCommand(sock, "RETR 1")) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }

    // 先接收第一行 +OK 响应
    string retrFirstLine = receiveResponse(sock);
    if (!isOK(retrFirstLine)) {
        cerr << "[错误] RETR 命令失败（可能邮箱为空）" << endl;
    } else {
        // 接收多行响应（完整的邮件内容，包括头部和正文）
        cout << "[信息] 邮件内容：" << endl;
        cout << "------------------------------------" << endl;
        string emailContent = receiveMultilineResponse(sock);
        cout << emailContent << endl;
        cout << "------------------------------------" << endl;
    }

    // Step 11: （可选）发送 DELE 命令 - 标记删除邮件
    // 注意：这里注释掉了删除操作，以免误删邮件
    // 如果需要测试删除功能，请取消下面的注释
    /*
    cout << "\n[4] 标记删除第一封邮件 (DELE 1 命令)" << endl;
    if (!sendCommand(sock, "DELE 1")) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    string deleResponse = receiveResponse(sock);
    if (!isOK(deleResponse)) {
        cerr << "[错误] DELE 命令失败" << endl;
    } else {
        cout << "[信息] 邮件已标记为删除（在 QUIT 后才会真正删除）" << endl;
    }
    */

    // ========================================================================
    // UPDATE 状态：发送 QUIT 命令，服务器执行清理操作
    // ========================================================================

    cout << "\n--- 进入更新状态 (UPDATE 状态) ---" << endl;

    // Step 12: 发送 QUIT 命令 - 退出并提交更改
    cout << "\n[5] 退出并提交更改 (QUIT 命令)" << endl;
    if (!sendCommand(sock, "QUIT")) {
        closesocket(sock);
        cleanupSocket();
        return 1;
    }
    string quitResponse = receiveResponse(sock);
    if (!isOK(quitResponse)) {
        cerr << "[错误] QUIT 命令失败" << endl;
    } else {
        cout << "[信息] 已成功退出，所有标记删除的邮件将被永久删除" << endl;
    }

    // Step 13: 关闭连接
    closesocket(sock);
    cleanupSocket();

    cout << "\n========================================" << endl;
    cout << "          POP3 会话结束" << endl;
    cout << "========================================" << endl;

    return 0;
}
