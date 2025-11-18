/*
 * SMTP 客户端 - 学习用途
 *
 * 功能：使用底层 TCP Socket 实现 SMTP 协议，发送电子邮件
 * 注意：此实现不包含 SSL/TLS 加密，仅用于学习 SMTP 协议原理
 *
 * SMTP 协议简介：
 * SMTP (Simple Mail Transfer Protocol) 是一个纯文本的"一问一答"协议
 * 客户端发送命令，服务器返回状态码（如 250 OK, 354 Start mail input 等）
 *
 * 警告：现代公共邮件服务器（如 Gmail, 163.com）都要求 SSL/TLS 和认证
 *       此示例代码仅适用于本地测试服务器或特定的无加密SMTP服务器
 */

#include <iostream>
#include <string>
#include <cstring>
#include <sstream>

// POSIX Socket 头文件 (Linux/Unix)
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
#endif

using namespace std;

// ==================== 配置参数 ====================
// 可以根据需要修改这些参数
const string SMTP_SERVER = "smtp.example.com";  // SMTP 服务器地址
const int SMTP_PORT = 25;                        // SMTP 端口（25 为未加密端口）
const string FROM_EMAIL = "from@example.com";   // 发件人邮箱
const string TO_EMAIL = "to@example.com";       // 收件人邮箱
const string FROM_NAME = "发件人名称";           // 发件人显示名称
const string TO_NAME = "收件人名称";             // 收件人显示名称

// 如果需要认证（AUTH LOGIN），取消注释并填写
// const string USERNAME = "your_username";      // 邮箱用户名
// const string PASSWORD = "your_password";      // 邮箱密码
const bool NEED_AUTH = false;                    // 是否需要认证

// ==================== Base64 编码函数 ====================
/**
 * Base64 编码
 * 用于 SMTP AUTH LOGIN 时编码用户名和密码
 *
 * Base64 原理：将每3个字节（24位）分成4组，每组6位，
 * 然后用一个可打印字符表示这6位的值（0-63）
 */
string base64_encode(const string& input) {
    static const char base64_chars[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    string output;
    int val = 0;
    int valb = -6;

    // 遍历输入的每个字节
    for (unsigned char c : input) {
        val = (val << 8) + c;  // 累积位
        valb += 8;

        // 每当累积够6位，就输出一个base64字符
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }

    // 处理剩余的位
    if (valb > -6) {
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }

    // 添加填充字符 '='
    while (output.size() % 4) {
        output.push_back('=');
    }

    return output;
}

// ==================== Socket 辅助函数 ====================

/**
 * 发送数据到服务器
 * @param sock: socket 文件描述符
 * @param data: 要发送的数据
 * @return: 成功返回 true，失败返回 false
 */
bool send_data(SOCKET sock, const string& data) {
    cout << ">>> 发送: " << data;  // 显示发送的内容
    if (data.back() != '\n') {
        cout << endl;
    }

    int bytes_sent = send(sock, data.c_str(), data.length(), 0);
    if (bytes_sent == SOCKET_ERROR) {
        cerr << "发送失败！" << endl;
        return false;
    }
    return true;
}

/**
 * 接收服务器响应
 * @param sock: socket 文件描述符
 * @param response: 存储接收到的响应
 * @return: 成功返回 true，失败返回 false
 */
bool recv_response(SOCKET sock, string& response) {
    char buffer[4096] = {0};
    int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received == SOCKET_ERROR || bytes_received == 0) {
        cerr << "接收失败或连接已关闭！" << endl;
        return false;
    }

    buffer[bytes_received] = '\0';
    response = buffer;
    cout << "<<< 接收: " << response;  // 显示接收的内容
    if (response.back() != '\n') {
        cout << endl;
    }

    return true;
}

/**
 * 检查 SMTP 响应码
 * @param response: 服务器响应
 * @param expected_code: 期望的响应码（如 "250", "354"）
 * @return: 响应码匹配返回 true，否则返回 false
 */
bool check_response(const string& response, const string& expected_code) {
    // SMTP 响应格式：状态码 + 空格/横杠 + 说明文字
    // 例如："250 OK" 或 "250-AUTH LOGIN PLAIN"
    if (response.length() >= 3 && response.substr(0, 3) == expected_code) {
        return true;
    }
    cerr << "错误：期望响应码 " << expected_code << "，但收到: " << response << endl;
    return false;
}

// ==================== 主函数 ====================
int main() {
    cout << "========================================" << endl;
    cout << "    SMTP 客户端 - 学习演示程序" << endl;
    cout << "========================================" << endl;
    cout << endl;

#ifdef _WIN32
    // Windows 平台需要初始化 Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 失败！" << endl;
        return 1;
    }
#endif

    SOCKET smtp_socket = INVALID_SOCKET;
    string response;

    try {
        // ========== 步骤 1: 解析服务器地址 ==========
        cout << "[步骤 1] 解析服务器地址: " << SMTP_SERVER << endl;

        struct hostent* host = gethostbyname(SMTP_SERVER.c_str());
        if (host == nullptr) {
            throw runtime_error("无法解析主机名: " + SMTP_SERVER);
        }

        // 将解析到的 IP 地址转换为字符串显示
        char* ip = inet_ntoa(*((struct in_addr*)host->h_addr));
        cout << "    解析到 IP: " << ip << endl;
        cout << endl;

        // ========== 步骤 2: 创建 TCP Socket ==========
        cout << "[步骤 2] 创建 TCP Socket" << endl;

        // SOCK_STREAM 表示 TCP 协议
        smtp_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (smtp_socket == INVALID_SOCKET) {
            throw runtime_error("创建 socket 失败！");
        }
        cout << "    Socket 创建成功" << endl;
        cout << endl;

        // ========== 步骤 3: 连接到 SMTP 服务器 ==========
        cout << "[步骤 3] 连接到 SMTP 服务器: " << SMTP_SERVER << ":" << SMTP_PORT << endl;

        struct sockaddr_in server_addr;
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SMTP_PORT);  // 主机字节序转网络字节序
        memcpy(&server_addr.sin_addr, host->h_addr, host->h_length);

        if (connect(smtp_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            throw runtime_error("连接服务器失败！");
        }
        cout << "    连接成功" << endl;
        cout << endl;

        // ========== 步骤 4: 接收服务器欢迎消息 ==========
        // SMTP 服务器在连接建立后会立即发送 220 欢迎消息
        cout << "[步骤 4] 接收服务器欢迎消息（期望 220）" << endl;
        if (!recv_response(smtp_socket, response) || !check_response(response, "220")) {
            throw runtime_error("未收到正确的欢迎消息！");
        }
        cout << endl;

        // ========== 步骤 5: 发送 HELO 命令 ==========
        // HELO 命令用于向服务器标识客户端
        // 格式：HELO <客户端域名或标识>
        cout << "[步骤 5] 发送 HELO 命令" << endl;
        if (!send_data(smtp_socket, "HELO world\r\n")) {
            throw runtime_error("发送 HELO 失败！");
        }
        if (!recv_response(smtp_socket, response) || !check_response(response, "250")) {
            throw runtime_error("HELO 命令失败！");
        }
        cout << endl;

        // ========== 步骤 6（可选）: AUTH LOGIN 认证 ==========
        // 如果服务器需要认证，这里演示 AUTH LOGIN 流程
        // 注意：AUTH LOGIN 使用 Base64 编码传输用户名和密码
        if (NEED_AUTH) {
            cout << "[步骤 6] 进行 AUTH LOGIN 认证" << endl;

            // 6.1 发送 AUTH LOGIN 命令
            if (!send_data(smtp_socket, "AUTH LOGIN\r\n")) {
                throw runtime_error("发送 AUTH LOGIN 失败！");
            }
            // 服务器返回 334 VXNlcm5hbWU6 (Base64 编码的 "Username:")
            if (!recv_response(smtp_socket, response) || !check_response(response, "334")) {
                throw runtime_error("AUTH LOGIN 初始化失败！");
            }

            // 6.2 发送 Base64 编码的用户名
            // string username_b64 = base64_encode(USERNAME);
            // if (!send_data(smtp_socket, username_b64 + "\r\n")) {
            //     throw runtime_error("发送用户名失败！");
            // }
            // if (!recv_response(smtp_socket, response) || !check_response(response, "334")) {
            //     throw runtime_error("用户名验证失败！");
            // }

            // 6.3 发送 Base64 编码的密码
            // string password_b64 = base64_encode(PASSWORD);
            // if (!send_data(smtp_socket, password_b64 + "\r\n")) {
            //     throw runtime_error("发送密码失败！");
            // }
            // if (!recv_response(smtp_socket, response) || !check_response(response, "235")) {
            //     throw runtime_error("密码验证失败！");
            // }

            cout << "    认证成功" << endl;
            cout << endl;
        }

        // ========== 步骤 7: 发送 MAIL FROM 命令 ==========
        // 指定发件人地址
        // 格式：MAIL FROM: <邮箱地址>
        cout << "[步骤 7] 发送 MAIL FROM 命令" << endl;
        if (!send_data(smtp_socket, "MAIL FROM: <" + FROM_EMAIL + ">\r\n")) {
            throw runtime_error("发送 MAIL FROM 失败！");
        }
        if (!recv_response(smtp_socket, response) || !check_response(response, "250")) {
            throw runtime_error("MAIL FROM 命令失败！");
        }
        cout << endl;

        // ========== 步骤 8: 发送 RCPT TO 命令 ==========
        // 指定收件人地址（可以有多个 RCPT TO 命令，对应多个收件人）
        // 格式：RCPT TO: <邮箱地址>
        cout << "[步骤 8] 发送 RCPT TO 命令" << endl;
        if (!send_data(smtp_socket, "RCPT TO: <" + TO_EMAIL + ">\r\n")) {
            throw runtime_error("发送 RCPT TO 失败！");
        }
        if (!recv_response(smtp_socket, response) || !check_response(response, "250")) {
            throw runtime_error("RCPT TO 命令失败！");
        }
        cout << endl;

        // ========== 步骤 9: 发送 DATA 命令 ==========
        // DATA 命令表示准备发送邮件内容
        // 服务器会返回 354，表示可以开始输入邮件内容
        cout << "[步骤 9] 发送 DATA 命令" << endl;
        if (!send_data(smtp_socket, "DATA\r\n")) {
            throw runtime_error("发送 DATA 失败！");
        }
        if (!recv_response(smtp_socket, response) || !check_response(response, "354")) {
            throw runtime_error("DATA 命令失败！");
        }
        cout << endl;

        // ========== 步骤 10: 发送邮件内容 ==========
        // 邮件内容包括：邮件头（Header）+ 空行 + 邮件体（Body）
        //
        // 重要：邮件头中的 From、To、Subject 是邮件显示的发件人/收件人/主题
        //       它们与 SMTP 命令中的 MAIL FROM 和 RCPT TO 不同！
        //
        // 结束标志：单独一行只有一个点号 ".\r\n"
        cout << "[步骤 10] 发送邮件内容" << endl;

        stringstream email_content;

        // 邮件头
        email_content << "From: " << FROM_NAME << " <" << FROM_EMAIL << ">\r\n";
        email_content << "To: " << TO_NAME << " <" << TO_EMAIL << ">\r\n";
        email_content << "Subject: Hello from C++ SMTP Client!\r\n";
        email_content << "Content-Type: text/plain; charset=utf-8\r\n";  // 指定内容类型和编码

        // 空行分隔邮件头和邮件体
        email_content << "\r\n";

        // 邮件体
        email_content << "这是一封来自 C++ SMTP 客户端的测试邮件。\r\n";
        email_content << "\r\n";
        email_content << "这个程序使用底层的 TCP Socket API 实现了 SMTP 协议。\r\n";
        email_content << "SMTP 协议是一个简单的文本协议，通过一问一答的方式与服务器交互。\r\n";
        email_content << "\r\n";
        email_content << "主要步骤包括：\r\n";
        email_content << "1. 连接服务器（TCP Socket）\r\n";
        email_content << "2. HELO 握手\r\n";
        email_content << "3. （可选）AUTH LOGIN 认证\r\n";
        email_content << "4. MAIL FROM 指定发件人\r\n";
        email_content << "5. RCPT TO 指定收件人\r\n";
        email_content << "6. DATA 发送邮件内容\r\n";
        email_content << "7. QUIT 结束会话\r\n";
        email_content << "\r\n";
        email_content << "祝你学习愉快！\r\n";
        email_content << "\r\n";
        email_content << "-- \r\n";
        email_content << "C++ SMTP Client\r\n";

        // 发送邮件内容
        if (!send_data(smtp_socket, email_content.str())) {
            throw runtime_error("发送邮件内容失败！");
        }

        // ========== 步骤 11: 发送结束标志 ==========
        // 重要：必须发送 "\r\n.\r\n" 来标识邮件内容结束
        // 这是 SMTP 协议的规定
        cout << "[步骤 11] 发送邮件结束标志（\\r\\n.\\r\\n）" << endl;
        if (!send_data(smtp_socket, "\r\n.\r\n")) {
            throw runtime_error("发送结束标志失败！");
        }
        // 服务器返回 250，表示邮件已接受并加入发送队列
        if (!recv_response(smtp_socket, response) || !check_response(response, "250")) {
            throw runtime_error("邮件发送失败！");
        }
        cout << endl;

        // ========== 步骤 12: 发送 QUIT 命令 ==========
        // QUIT 命令用于正常结束 SMTP 会话
        cout << "[步骤 12] 发送 QUIT 命令" << endl;
        if (!send_data(smtp_socket, "QUIT\r\n")) {
            throw runtime_error("发送 QUIT 失败！");
        }
        if (!recv_response(smtp_socket, response) || !check_response(response, "221")) {
            throw runtime_error("QUIT 命令失败！");
        }
        cout << endl;

        cout << "========================================" << endl;
        cout << "    邮件发送成功！" << endl;
        cout << "========================================" << endl;

    } catch (const exception& e) {
        cerr << endl;
        cerr << "========================================" << endl;
        cerr << "    发生错误: " << e.what() << endl;
        cerr << "========================================" << endl;

        if (smtp_socket != INVALID_SOCKET) {
            closesocket(smtp_socket);
        }

#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }

    // ========== 步骤 13: 关闭连接 ==========
    closesocket(smtp_socket);

#ifdef _WIN32
    WSACleanup();
#endif

    return 0;
}
