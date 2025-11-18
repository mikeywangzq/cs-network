/*
 * Peer 客户端 - 简化版 P2P 文件分享工具
 * 功能：
 *   1. 作为服务器：监听其他 Peer 的连接，上传文件片段
 *   2. 作为客户端：连接其他 Peer，下载文件片段
 *   3. 多线程架构：服务器线程 + 多个下载线程
 * 协议：HANDSHAKE, BITFIELD, REQUEST, PIECE, HAVE
 */

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <thread>
#include <mutex>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/stat.h>

// ======================== 常量定义 ========================
const int PIECE_SIZE = 65536;  // 64KB 片段大小
const int BUFFER_SIZE = 8192;  // 缓冲区大小
const int DEFAULT_TRACKER_PORT = 6881;

// ======================== 全局变量 ========================
std::string g_file_id;              // 文件 ID
std::string g_file_path;            // 文件路径
long long g_file_size = 0;          // 文件大小
int g_num_pieces = 0;               // 片段总数
int g_listen_port = 0;              // 本地监听端口
std::string g_tracker_ip;           // Tracker IP
int g_tracker_port = DEFAULT_TRACKER_PORT;  // Tracker 端口

// 位域：bitfield[i] = true 表示拥有第 i 个片段
std::vector<bool> g_bitfield;
std::mutex g_bitfield_mutex;

// 已连接的 Peer 列表（IP:Port -> socket）
std::map<std::string, int> g_connected_peers;
std::mutex g_peers_mutex;

// 已下载的片段集合
std::set<int> g_downloaded_pieces;
std::mutex g_file_mutex;  // 保护文件写入

// ======================== 辅助函数 ========================

/**
 * 分割字符串
 */
std::vector<std::string> split(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}

/**
 * 去除字符串首尾空白
 */
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

/**
 * 位域转十六进制字符串
 * 用于向 Tracker 注册时发送位域信息
 */
std::string bitfield_to_hex(const std::vector<bool>& bitfield) {
    int num_bytes = (bitfield.size() + 7) / 8;
    std::vector<unsigned char> bytes(num_bytes, 0);

    for (size_t i = 0; i < bitfield.size(); i++) {
        if (bitfield[i]) {
            bytes[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    std::stringstream ss;
    for (unsigned char byte : bytes) {
        char hex[3];
        snprintf(hex, sizeof(hex), "%02X", byte);
        ss << hex;
    }

    return ss.str();
}

/**
 * 十六进制字符串转位域
 */
std::vector<bool> hex_to_bitfield(const std::string& hex, int num_pieces) {
    std::vector<bool> bitfield(num_pieces, false);

    for (size_t i = 0; i < hex.length(); i += 2) {
        if (i + 1 >= hex.length()) break;

        std::string byte_str = hex.substr(i, 2);
        unsigned char byte = (unsigned char)strtol(byte_str.c_str(), nullptr, 16);

        for (int bit = 0; bit < 8; bit++) {
            int piece_index = (i / 2) * 8 + bit;
            if (piece_index >= num_pieces) break;
            if (byte & (1 << (7 - bit))) {
                bitfield[piece_index] = true;
            }
        }
    }

    return bitfield;
}

/**
 * 位域转二进制数据（用于 P2P BITFIELD 消息）
 */
std::vector<unsigned char> bitfield_to_binary(const std::vector<bool>& bitfield) {
    int num_bytes = (bitfield.size() + 7) / 8;
    std::vector<unsigned char> bytes(num_bytes, 0);

    for (size_t i = 0; i < bitfield.size(); i++) {
        if (bitfield[i]) {
            bytes[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    return bytes;
}

/**
 * 二进制数据转位域
 */
std::vector<bool> binary_to_bitfield(const unsigned char* data, int num_pieces) {
    std::vector<bool> bitfield(num_pieces, false);

    for (int i = 0; i < num_pieces; i++) {
        int byte_index = i / 8;
        int bit_index = 7 - (i % 8);
        if (data[byte_index] & (1 << bit_index)) {
            bitfield[i] = true;
        }
    }

    return bitfield;
}

/**
 * 完整接收指定字节数的数据
 * 处理 recv 可能返回部分数据的情况
 */
bool recv_full(int socket, void* buffer, size_t length) {
    size_t total_received = 0;
    char* buf = (char*)buffer;

    while (total_received < length) {
        ssize_t received = recv(socket, buf + total_received, length - total_received, 0);
        if (received <= 0) {
            return false;  // 连接断开或错误
        }
        total_received += received;
    }

    return true;
}

/**
 * 完整发送指定字节数的数据
 * 处理 send 可能返回部分数据的情况
 */
bool send_full(int socket, const void* buffer, size_t length) {
    size_t total_sent = 0;
    const char* buf = (const char*)buffer;

    while (total_sent < length) {
        ssize_t sent = send(socket, buf + total_sent, length - total_sent, 0);
        if (sent <= 0) {
            return false;  // 连接断开或错误
        }
        total_sent += sent;
    }

    return true;
}

/**
 * 接收一行文本（以 \n 结尾）
 */
bool recv_line(int socket, std::string& line) {
    line.clear();
    char ch;

    while (true) {
        ssize_t received = recv(socket, &ch, 1, 0);
        if (received <= 0) {
            return false;
        }
        if (ch == '\n') {
            break;
        }
        line += ch;
    }

    return true;
}

// ======================== Tracker 通信 ========================

/**
 * 向 Tracker 注册自己
 */
bool register_to_tracker() {
    std::cout << "[INFO] Registering to Tracker..." << std::endl;

    // 连接 Tracker
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return false;
    }

    struct sockaddr_in tracker_addr;
    memset(&tracker_addr, 0, sizeof(tracker_addr));
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(g_tracker_port);

    if (inet_pton(AF_INET, g_tracker_ip.c_str(), &tracker_addr.sin_addr) <= 0) {
        std::cerr << "[ERROR] Invalid Tracker IP" << std::endl;
        close(sock);
        return false;
    }

    if (connect(sock, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr)) < 0) {
        std::cerr << "[ERROR] Failed to connect to Tracker" << std::endl;
        close(sock);
        return false;
    }

    // 构建 REGISTER 命令
    std::string bitfield_hex = bitfield_to_hex(g_bitfield);
    std::stringstream ss;
    ss << "REGISTER " << g_file_id << " " << g_listen_port << " " << bitfield_hex << "\n";
    std::string command = ss.str();

    // 发送命令
    if (!send_full(sock, command.c_str(), command.size())) {
        std::cerr << "[ERROR] Failed to send REGISTER command" << std::endl;
        close(sock);
        return false;
    }

    // 接收响应
    std::string response;
    if (!recv_line(sock, response)) {
        std::cerr << "[ERROR] Failed to receive response from Tracker" << std::endl;
        close(sock);
        return false;
    }

    close(sock);

    response = trim(response);
    if (response == "OK") {
        std::cout << "[INFO] Successfully registered to Tracker" << std::endl;
        return true;
    } else {
        std::cerr << "[ERROR] Tracker returned: " << response << std::endl;
        return false;
    }
}

/**
 * 从 Tracker 获取 Peer 列表
 */
std::vector<std::string> get_peers_from_tracker() {
    std::vector<std::string> peers;

    // 连接 Tracker
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return peers;
    }

    struct sockaddr_in tracker_addr;
    memset(&tracker_addr, 0, sizeof(tracker_addr));
    tracker_addr.sin_family = AF_INET;
    tracker_addr.sin_port = htons(g_tracker_port);

    if (inet_pton(AF_INET, g_tracker_ip.c_str(), &tracker_addr.sin_addr) <= 0) {
        std::cerr << "[ERROR] Invalid Tracker IP" << std::endl;
        close(sock);
        return peers;
    }

    if (connect(sock, (struct sockaddr*)&tracker_addr, sizeof(tracker_addr)) < 0) {
        std::cerr << "[ERROR] Failed to connect to Tracker" << std::endl;
        close(sock);
        return peers;
    }

    // 构建 GETPEERS 命令
    std::string command = "GETPEERS " + g_file_id + "\n";

    // 发送命令
    if (!send_full(sock, command.c_str(), command.size())) {
        std::cerr << "[ERROR] Failed to send GETPEERS command" << std::endl;
        close(sock);
        return peers;
    }

    // 接收响应
    std::string response;
    if (!recv_line(sock, response)) {
        std::cerr << "[ERROR] Failed to receive response from Tracker" << std::endl;
        close(sock);
        return peers;
    }

    close(sock);

    response = trim(response);

    // 解析响应：PEERS ip1:port1,ip2:port2,...
    std::vector<std::string> tokens = split(response, ' ');
    if (tokens.size() >= 2 && tokens[0] == "PEERS") {
        std::string peers_str = tokens[1];
        if (!peers_str.empty()) {
            peers = split(peers_str, ',');
        }
    }

    std::cout << "[INFO] Got " << peers.size() << " peers from Tracker" << std::endl;

    return peers;
}

// ======================== 文件操作 ========================

/**
 * 读取文件的指定片段
 */
bool read_piece(int piece_index, std::vector<char>& data) {
    std::lock_guard<std::mutex> lock(g_file_mutex);

    std::ifstream file(g_file_path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    long long offset = (long long)piece_index * PIECE_SIZE;
    file.seekg(offset);

    // 计算该片段的实际大小
    int piece_size = PIECE_SIZE;
    if (piece_index == g_num_pieces - 1) {
        // 最后一个片段可能不足 PIECE_SIZE
        piece_size = g_file_size - offset;
    }

    data.resize(piece_size);
    file.read(data.data(), piece_size);

    file.close();

    return file.gcount() == piece_size;
}

/**
 * 写入文件的指定片段
 */
bool write_piece(int piece_index, const char* data, int size) {
    std::lock_guard<std::mutex> lock(g_file_mutex);

    // 以读写模式打开文件（如果不存在则创建）
    std::fstream file(g_file_path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) {
        // 文件不存在，创建文件
        std::ofstream new_file(g_file_path, std::ios::binary);
        if (!new_file.is_open()) {
            return false;
        }
        // 预分配文件大小
        new_file.seekp(g_file_size - 1);
        new_file.put(0);
        new_file.close();

        // 重新打开
        file.open(g_file_path, std::ios::in | std::ios::out | std::ios::binary);
        if (!file.is_open()) {
            return false;
        }
    }

    long long offset = (long long)piece_index * PIECE_SIZE;
    file.seekp(offset);
    file.write(data, size);

    file.close();

    return true;
}

/**
 * 检查是否已完成所有片段的下载
 */
bool is_download_complete() {
    std::lock_guard<std::mutex> lock(g_bitfield_mutex);
    for (int i = 0; i < g_num_pieces; i++) {
        if (!g_bitfield[i]) {
            return false;
        }
    }
    return true;
}

// ======================== P2P 协议 - 服务器端 ========================

/**
 * 处理来自其他 Peer 的连接（服务器角色）
 * 这个函数实现了 P2P 协议的服务器端
 */
void handle_peer_connection(int peer_socket, const std::string& peer_ip) {
    std::cout << "[SERVER] Handling connection from " << peer_ip << std::endl;

    // 1. 接收 HANDSHAKE
    std::string handshake;
    if (!recv_line(peer_socket, handshake)) {
        std::cerr << "[SERVER] Failed to receive handshake" << std::endl;
        close(peer_socket);
        return;
    }

    handshake = trim(handshake);
    std::vector<std::string> tokens = split(handshake, ' ');

    if (tokens.size() < 2 || tokens[0] != "HANDSHAKE") {
        std::cerr << "[SERVER] Invalid handshake" << std::endl;
        close(peer_socket);
        return;
    }

    std::string requested_file_id = tokens[1];

    // 验证文件 ID
    if (requested_file_id != g_file_id) {
        std::string error_msg = "ERROR Wrong file_id\n";
        send_full(peer_socket, error_msg.c_str(), error_msg.size());
        close(peer_socket);
        return;
    }

    // 2. 发送 HANDSHAKE_OK
    std::string response = "HANDSHAKE_OK\n";
    if (!send_full(peer_socket, response.c_str(), response.size())) {
        std::cerr << "[SERVER] Failed to send HANDSHAKE_OK" << std::endl;
        close(peer_socket);
        return;
    }

    // 3. 交换 BITFIELD
    // 首先发送自己的位域
    {
        std::lock_guard<std::mutex> lock(g_bitfield_mutex);
        std::vector<unsigned char> bitfield_data = bitfield_to_binary(g_bitfield);
        std::string bitfield_header = "BITFIELD " + std::to_string(bitfield_data.size()) + "\n";

        if (!send_full(peer_socket, bitfield_header.c_str(), bitfield_header.size())) {
            std::cerr << "[SERVER] Failed to send BITFIELD header" << std::endl;
            close(peer_socket);
            return;
        }

        if (!send_full(peer_socket, bitfield_data.data(), bitfield_data.size())) {
            std::cerr << "[SERVER] Failed to send BITFIELD data" << std::endl;
            close(peer_socket);
            return;
        }
    }

    // 接收对方的位域（但我们作为服务器端不需要主动下载，所以可以忽略）
    std::string bitfield_header;
    if (!recv_line(peer_socket, bitfield_header)) {
        std::cerr << "[SERVER] Failed to receive BITFIELD header" << std::endl;
        close(peer_socket);
        return;
    }

    tokens = split(trim(bitfield_header), ' ');
    if (tokens.size() < 2 || tokens[0] != "BITFIELD") {
        std::cerr << "[SERVER] Invalid BITFIELD header" << std::endl;
        close(peer_socket);
        return;
    }

    int bitfield_size = std::atoi(tokens[1].c_str());
    std::vector<unsigned char> peer_bitfield_data(bitfield_size);

    if (!recv_full(peer_socket, peer_bitfield_data.data(), bitfield_size)) {
        std::cerr << "[SERVER] Failed to receive BITFIELD data" << std::endl;
        close(peer_socket);
        return;
    }

    // 4. 主循环：处理 REQUEST 和发送 PIECE
    while (true) {
        std::string command;
        if (!recv_line(peer_socket, command)) {
            // 连接断开
            break;
        }

        command = trim(command);
        if (command.empty()) continue;

        tokens = split(command, ' ');
        if (tokens.empty()) continue;

        if (tokens[0] == "REQUEST" && tokens.size() >= 2) {
            int piece_index = std::atoi(tokens[1].c_str());

            std::cout << "[SERVER] Peer " << peer_ip << " requests piece " << piece_index << std::endl;

            // 检查是否拥有该片段
            bool has_piece = false;
            {
                std::lock_guard<std::mutex> lock(g_bitfield_mutex);
                if (piece_index >= 0 && piece_index < g_num_pieces) {
                    has_piece = g_bitfield[piece_index];
                }
            }

            if (!has_piece) {
                std::string error_msg = "ERROR Piece not available\n";
                send_full(peer_socket, error_msg.c_str(), error_msg.size());
                continue;
            }

            // 读取片段数据
            std::vector<char> piece_data;
            if (!read_piece(piece_index, piece_data)) {
                std::string error_msg = "ERROR Failed to read piece\n";
                send_full(peer_socket, error_msg.c_str(), error_msg.size());
                continue;
            }

            // 发送 PIECE 头部
            std::stringstream ss;
            ss << "PIECE " << piece_index << " " << piece_data.size() << "\n";
            std::string piece_header = ss.str();

            if (!send_full(peer_socket, piece_header.c_str(), piece_header.size())) {
                std::cerr << "[SERVER] Failed to send PIECE header" << std::endl;
                break;
            }

            // 发送片段数据
            if (!send_full(peer_socket, piece_data.data(), piece_data.size())) {
                std::cerr << "[SERVER] Failed to send PIECE data" << std::endl;
                break;
            }

            std::cout << "[SERVER] Sent piece " << piece_index << " to " << peer_ip << std::endl;

        } else if (tokens[0] == "HAVE" && tokens.size() >= 2) {
            // 对方通知它有了新片段（我们可以记录，但在这个简化版本中暂时忽略）
            int piece_index = std::atoi(tokens[1].c_str());
            std::cout << "[SERVER] Peer " << peer_ip << " now has piece " << piece_index << std::endl;
        }
    }

    std::cout << "[SERVER] Connection closed with " << peer_ip << std::endl;
    close(peer_socket);
}

/**
 * 服务器线程：监听其他 Peer 的连接
 */
void server_thread_func() {
    std::cout << "[SERVER] Server thread started, listening on port " << g_listen_port << std::endl;

    // 创建 socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "[SERVER] Failed to create socket" << std::endl;
        return;
    }

    // 设置 socket 选项
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(g_listen_port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[SERVER] Bind failed" << std::endl;
        close(server_socket);
        return;
    }

    // 开始监听
    if (listen(server_socket, 10) < 0) {
        std::cerr << "[SERVER] Listen failed" << std::endl;
        close(server_socket);
        return;
    }

    std::cout << "[SERVER] Listening for incoming connections..." << std::endl;

    // 主循环：接受连接
    while (true) {
        struct sockaddr_in peer_addr;
        socklen_t peer_len = sizeof(peer_addr);

        int peer_socket = accept(server_socket, (struct sockaddr*)&peer_addr, &peer_len);
        if (peer_socket < 0) {
            std::cerr << "[SERVER] Accept failed" << std::endl;
            continue;
        }

        // 获取 Peer IP
        char peer_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &peer_addr.sin_addr, peer_ip, INET_ADDRSTRLEN);

        std::cout << "[SERVER] New connection from " << peer_ip << std::endl;

        // 为每个连接创建一个线程
        std::thread t(handle_peer_connection, peer_socket, std::string(peer_ip));
        t.detach();
    }

    close(server_socket);
}

// ======================== P2P 协议 - 客户端 ========================

/**
 * 连接到一个 Peer 并下载缺失的片段（客户端角色）
 */
void download_from_peer(const std::string& peer_addr) {
    // 解析 IP:Port
    std::vector<std::string> tokens = split(peer_addr, ':');
    if (tokens.size() != 2) {
        std::cerr << "[CLIENT] Invalid peer address: " << peer_addr << std::endl;
        return;
    }

    std::string peer_ip = tokens[0];
    int peer_port = std::atoi(tokens[1].c_str());

    std::cout << "[CLIENT] Connecting to peer " << peer_ip << ":" << peer_port << std::endl;

    // 创建 socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "[CLIENT] Failed to create socket" << std::endl;
        return;
    }

    // 连接到 Peer
    struct sockaddr_in peer_addr_struct;
    memset(&peer_addr_struct, 0, sizeof(peer_addr_struct));
    peer_addr_struct.sin_family = AF_INET;
    peer_addr_struct.sin_port = htons(peer_port);

    if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr_struct.sin_addr) <= 0) {
        std::cerr << "[CLIENT] Invalid peer IP" << std::endl;
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr*)&peer_addr_struct, sizeof(peer_addr_struct)) < 0) {
        std::cerr << "[CLIENT] Failed to connect to " << peer_addr << std::endl;
        close(sock);
        return;
    }

    std::cout << "[CLIENT] Connected to " << peer_addr << std::endl;

    // 1. 发送 HANDSHAKE
    std::string handshake = "HANDSHAKE " + g_file_id + "\n";
    if (!send_full(sock, handshake.c_str(), handshake.size())) {
        std::cerr << "[CLIENT] Failed to send HANDSHAKE" << std::endl;
        close(sock);
        return;
    }

    // 接收 HANDSHAKE_OK
    std::string response;
    if (!recv_line(sock, response)) {
        std::cerr << "[CLIENT] Failed to receive HANDSHAKE response" << std::endl;
        close(sock);
        return;
    }

    response = trim(response);
    if (response != "HANDSHAKE_OK") {
        std::cerr << "[CLIENT] Handshake failed: " << response << std::endl;
        close(sock);
        return;
    }

    std::cout << "[CLIENT] Handshake successful with " << peer_addr << std::endl;

    // 2. 交换 BITFIELD
    // 先接收对方的位域
    std::string bitfield_header;
    if (!recv_line(sock, bitfield_header)) {
        std::cerr << "[CLIENT] Failed to receive BITFIELD header" << std::endl;
        close(sock);
        return;
    }

    std::vector<std::string> bf_tokens = split(trim(bitfield_header), ' ');
    if (bf_tokens.size() < 2 || bf_tokens[0] != "BITFIELD") {
        std::cerr << "[CLIENT] Invalid BITFIELD header" << std::endl;
        close(sock);
        return;
    }

    int bitfield_size = std::atoi(bf_tokens[1].c_str());
    std::vector<unsigned char> peer_bitfield_data(bitfield_size);

    if (!recv_full(sock, peer_bitfield_data.data(), bitfield_size)) {
        std::cerr << "[CLIENT] Failed to receive BITFIELD data" << std::endl;
        close(sock);
        return;
    }

    // 解析对方的位域
    std::vector<bool> peer_bitfield = binary_to_bitfield(peer_bitfield_data.data(), g_num_pieces);

    // 发送自己的位域
    {
        std::lock_guard<std::mutex> lock(g_bitfield_mutex);
        std::vector<unsigned char> my_bitfield_data = bitfield_to_binary(g_bitfield);
        std::string my_bitfield_header = "BITFIELD " + std::to_string(my_bitfield_data.size()) + "\n";

        if (!send_full(sock, my_bitfield_header.c_str(), my_bitfield_header.size())) {
            std::cerr << "[CLIENT] Failed to send BITFIELD header" << std::endl;
            close(sock);
            return;
        }

        if (!send_full(sock, my_bitfield_data.data(), my_bitfield_data.size())) {
            std::cerr << "[CLIENT] Failed to send BITFIELD data" << std::endl;
            close(sock);
            return;
        }
    }

    std::cout << "[CLIENT] Bitfield exchanged with " << peer_addr << std::endl;

    // 3. 请求和下载片段
    // 找出对方有而我们没有的片段
    std::vector<int> needed_pieces;
    {
        std::lock_guard<std::mutex> lock(g_bitfield_mutex);
        for (int i = 0; i < g_num_pieces; i++) {
            if (peer_bitfield[i] && !g_bitfield[i]) {
                needed_pieces.push_back(i);
            }
        }
    }

    std::cout << "[CLIENT] Peer " << peer_addr << " has " << needed_pieces.size() << " pieces we need" << std::endl;

    // 逐个下载需要的片段
    for (int piece_index : needed_pieces) {
        // 再次检查是否已经从其他 Peer 下载了该片段
        {
            std::lock_guard<std::mutex> lock(g_bitfield_mutex);
            if (g_bitfield[piece_index]) {
                continue;  // 已经有了，跳过
            }
        }

        // 发送 REQUEST
        std::string request = "REQUEST " + std::to_string(piece_index) + "\n";
        if (!send_full(sock, request.c_str(), request.size())) {
            std::cerr << "[CLIENT] Failed to send REQUEST" << std::endl;
            break;
        }

        // 接收 PIECE 头部
        std::string piece_header;
        if (!recv_line(sock, piece_header)) {
            std::cerr << "[CLIENT] Failed to receive PIECE header" << std::endl;
            break;
        }

        std::vector<std::string> piece_tokens = split(trim(piece_header), ' ');
        if (piece_tokens.size() < 3 || piece_tokens[0] != "PIECE") {
            std::cerr << "[CLIENT] Invalid PIECE header: " << piece_header << std::endl;
            break;
        }

        int received_piece_index = std::atoi(piece_tokens[1].c_str());
        int piece_data_size = std::atoi(piece_tokens[2].c_str());

        if (received_piece_index != piece_index) {
            std::cerr << "[CLIENT] Piece index mismatch" << std::endl;
            break;
        }

        // 接收片段数据
        std::vector<char> piece_data(piece_data_size);
        if (!recv_full(sock, piece_data.data(), piece_data_size)) {
            std::cerr << "[CLIENT] Failed to receive PIECE data" << std::endl;
            break;
        }

        // 写入文件
        if (!write_piece(piece_index, piece_data.data(), piece_data_size)) {
            std::cerr << "[CLIENT] Failed to write piece " << piece_index << std::endl;
            break;
        }

        // 更新位域
        {
            std::lock_guard<std::mutex> lock(g_bitfield_mutex);
            g_bitfield[piece_index] = true;
        }

        std::cout << "[CLIENT] Downloaded piece " << piece_index << " from " << peer_addr
                  << " (" << piece_data_size << " bytes)" << std::endl;

        // 发送 HAVE 通知
        std::string have_msg = "HAVE " + std::to_string(piece_index) + "\n";
        send_full(sock, have_msg.c_str(), have_msg.size());

        // 检查是否完成下载
        if (is_download_complete()) {
            std::cout << "[CLIENT] Download completed!" << std::endl;
            close(sock);
            return;
        }
    }

    close(sock);
    std::cout << "[CLIENT] Disconnected from " << peer_addr << std::endl;
}

/**
 * 下载器线程：周期性地从 Tracker 获取 Peer 列表并下载
 */
void downloader_thread_func() {
    std::cout << "[DOWNLOADER] Downloader thread started" << std::endl;

    while (!is_download_complete()) {
        // 从 Tracker 获取 Peer 列表
        std::vector<std::string> peers = get_peers_from_tracker();

        if (peers.empty()) {
            std::cout << "[DOWNLOADER] No peers available, waiting..." << std::endl;
            sleep(5);
            continue;
        }

        // 连接到每个 Peer 并下载
        for (const std::string& peer_addr : peers) {
            if (is_download_complete()) {
                break;
            }

            download_from_peer(peer_addr);
        }

        // 等待一段时间再重试
        if (!is_download_complete()) {
            sleep(3);
        }
    }

    std::cout << "[DOWNLOADER] Download completed! All pieces received." << std::endl;
}

// ======================== 主函数 ========================

/**
 * 显示使用说明
 */
void print_usage(const char* program_name) {
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << program_name << " <mode> <file_id> <file_path> <file_size> <listen_port> <tracker_ip> [tracker_port]" << std::endl;
    std::cout << std::endl;
    std::cout << "Parameters:" << std::endl;
    std::cout << "  mode         : 'seed' (有完整文件) 或 'download' (下载文件)" << std::endl;
    std::cout << "  file_id      : 文件标识符 (例如 'myfile')" << std::endl;
    std::cout << "  file_path    : 本地文件路径" << std::endl;
    std::cout << "  file_size    : 文件大小（字节）" << std::endl;
    std::cout << "  listen_port  : 本地监听端口" << std::endl;
    std::cout << "  tracker_ip   : Tracker 服务器 IP" << std::endl;
    std::cout << "  tracker_port : Tracker 服务器端口 (默认 6881)" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  # 作为种子节点（拥有完整文件）" << std::endl;
    std::cout << "  " << program_name << " seed myfile /tmp/file.dat 102400 7001 127.0.0.1" << std::endl;
    std::cout << std::endl;
    std::cout << "  # 作为下载节点" << std::endl;
    std::cout << "  " << program_name << " download myfile /tmp/file.dat 102400 7002 127.0.0.1" << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 7) {
        print_usage(argv[0]);
        return 1;
    }

    std::string mode = argv[1];
    g_file_id = argv[2];
    g_file_path = argv[3];
    g_file_size = std::atoll(argv[4]);
    g_listen_port = std::atoi(argv[5]);
    g_tracker_ip = argv[6];

    if (argc > 7) {
        g_tracker_port = std::atoi(argv[7]);
    }

    // 计算片段总数
    g_num_pieces = (g_file_size + PIECE_SIZE - 1) / PIECE_SIZE;

    // 初始化位域
    g_bitfield.resize(g_num_pieces, false);

    std::cout << "========================================" << std::endl;
    std::cout << "  Simple P2P File Sharing - Peer" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Mode        : " << mode << std::endl;
    std::cout << "File ID     : " << g_file_id << std::endl;
    std::cout << "File Path   : " << g_file_path << std::endl;
    std::cout << "File Size   : " << g_file_size << " bytes" << std::endl;
    std::cout << "Num Pieces  : " << g_num_pieces << std::endl;
    std::cout << "Piece Size  : " << PIECE_SIZE << " bytes" << std::endl;
    std::cout << "Listen Port : " << g_listen_port << std::endl;
    std::cout << "Tracker     : " << g_tracker_ip << ":" << g_tracker_port << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // 根据模式初始化
    if (mode == "seed") {
        // 种子模式：拥有完整文件
        std::cout << "[INFO] Running in SEED mode (have complete file)" << std::endl;

        // 检查文件是否存在
        std::ifstream file(g_file_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "[ERROR] File not found: " << g_file_path << std::endl;
            return 1;
        }
        file.close();

        // 设置所有位域为 true
        for (int i = 0; i < g_num_pieces; i++) {
            g_bitfield[i] = true;
        }

    } else if (mode == "download") {
        // 下载模式：没有文件，需要下载
        std::cout << "[INFO] Running in DOWNLOAD mode (downloading file)" << std::endl;

        // 所有位域初始为 false（已在初始化时设置）

    } else {
        std::cerr << "[ERROR] Invalid mode: " << mode << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // 向 Tracker 注册
    if (!register_to_tracker()) {
        std::cerr << "[ERROR] Failed to register to Tracker" << std::endl;
        return 1;
    }

    // 启动服务器线程
    std::thread server_thread(server_thread_func);

    // 如果是下载模式，启动下载器线程
    std::thread downloader_thread;
    if (mode == "download") {
        sleep(1);  // 等待服务器线程启动
        downloader_thread = std::thread(downloader_thread_func);
    }

    // 主线程等待
    if (mode == "seed") {
        std::cout << "[INFO] Seeding... Press Ctrl+C to stop." << std::endl;
        server_thread.join();
    } else {
        downloader_thread.join();
        std::cout << "[INFO] Download complete! Now seeding... Press Ctrl+C to stop." << std::endl;
        server_thread.join();
    }

    return 0;
}
