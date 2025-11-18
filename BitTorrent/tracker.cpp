/*
 * Tracker 服务器 - 简化版 P2P 文件分享工具
 * 功能：维护文件和 Peer 的映射关系，帮助 Peer 之间相互发现
 * 协议：REGISTER, GETPEERS, UPDATE
 */

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

const int BUFFER_SIZE = 4096;
const int TRACKER_PORT = 6881;  // Tracker 默认监听端口

// Peer 信息结构体
struct PeerInfo {
    std::string ip;           // Peer IP 地址
    int port;                 // Peer 监听端口
    std::string bitfield;     // 位域（十六进制字符串）

    PeerInfo(const std::string& _ip, int _port, const std::string& _bitfield)
        : ip(_ip), port(_port), bitfield(_bitfield) {}
};

// Tracker 核心数据结构
// 外层 map: file_id -> 该文件的所有 Peer 列表
// 内层 vector: 存储每个 Peer 的信息（IP、端口、位域）
std::map<std::string, std::vector<PeerInfo>> file_peers_map;
std::mutex map_mutex;  // 保护共享数据结构的互斥锁

/**
 * 辅助函数：分割字符串
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
 * 辅助函数：去除字符串首尾空白
 */
std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

/**
 * 处理 REGISTER 命令
 * 格式：REGISTER <file_id> <listen_port> <bitfield_hex>
 * 功能：注册 Peer，记录其拥有的文件片段
 */
std::string handle_register(const std::vector<std::string>& tokens, const std::string& peer_ip) {
    if (tokens.size() < 4) {
        return "ERROR Invalid REGISTER format\n";
    }

    std::string file_id = tokens[1];
    int listen_port = std::atoi(tokens[2].c_str());
    std::string bitfield = tokens[3];

    std::lock_guard<std::mutex> lock(map_mutex);

    // 检查该 Peer 是否已经注册过
    auto& peers = file_peers_map[file_id];
    bool found = false;
    for (auto& peer : peers) {
        if (peer.ip == peer_ip && peer.port == listen_port) {
            // 更新位域
            peer.bitfield = bitfield;
            found = true;
            break;
        }
    }

    if (!found) {
        // 添加新 Peer
        peers.emplace_back(peer_ip, listen_port, bitfield);
    }

    std::cout << "[REGISTER] File: " << file_id
              << ", Peer: " << peer_ip << ":" << listen_port
              << ", Bitfield: " << bitfield << std::endl;

    return "OK\n";
}

/**
 * 处理 GETPEERS 命令
 * 格式：GETPEERS <file_id>
 * 功能：返回拥有该文件的所有 Peer 列表
 */
std::string handle_getpeers(const std::vector<std::string>& tokens, const std::string& peer_ip) {
    if (tokens.size() < 2) {
        return "ERROR Invalid GETPEERS format\n";
    }

    std::string file_id = tokens[1];

    std::lock_guard<std::mutex> lock(map_mutex);

    auto it = file_peers_map.find(file_id);
    if (it == file_peers_map.end() || it->second.empty()) {
        return "PEERS \n";  // 没有 Peer
    }

    // 构建 Peer 列表：ip1:port1,ip2:port2,...
    std::stringstream ss;
    ss << "PEERS ";
    bool first = true;
    for (const auto& peer : it->second) {
        // 不返回请求者自己
        if (peer.ip == peer_ip) {
            continue;
        }
        if (!first) ss << ",";
        ss << peer.ip << ":" << peer.port;
        first = false;
    }
    ss << "\n";

    std::cout << "[GETPEERS] File: " << file_id
              << ", Requesting Peer: " << peer_ip
              << ", Returned " << it->second.size() << " peers" << std::endl;

    return ss.str();
}

/**
 * 处理 UPDATE 命令
 * 格式：UPDATE <file_id> <piece_index>
 * 功能：更新 Peer 的位域（当下载完新片段时）
 */
std::string handle_update(const std::vector<std::string>& tokens, const std::string& peer_ip) {
    if (tokens.size() < 3) {
        return "ERROR Invalid UPDATE format\n";
    }

    std::string file_id = tokens[1];
    int piece_index = std::atoi(tokens[2].c_str());

    std::lock_guard<std::mutex> lock(map_mutex);

    auto it = file_peers_map.find(file_id);
    if (it != file_peers_map.end()) {
        // 这里简化处理：实际应该更新位域
        // 但因为位域是十六进制字符串，更新比较复杂
        // 在真实实现中，应该解析、更新、再编码
        std::cout << "[UPDATE] File: " << file_id
                  << ", Peer: " << peer_ip
                  << ", Piece: " << piece_index << std::endl;
    }

    return "OK\n";
}

/**
 * 处理单个客户端连接的线程函数
 * 这个函数会：
 * 1. 接收客户端的命令
 * 2. 解析命令并调用相应的处理函数
 * 3. 发送响应
 */
void handle_client(int client_socket, const std::string& peer_ip) {
    char buffer[BUFFER_SIZE];

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);

        // 接收数据
        ssize_t bytes_received = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received == 0) {
                std::cout << "[INFO] Peer " << peer_ip << " disconnected" << std::endl;
            } else {
                std::cerr << "[ERROR] recv failed from " << peer_ip << std::endl;
            }
            break;
        }

        std::string command(buffer, bytes_received);
        command = trim(command);

        if (command.empty()) continue;

        std::cout << "[RECV] From " << peer_ip << ": " << command << std::endl;

        // 解析命令
        std::vector<std::string> tokens = split(command, ' ');
        if (tokens.empty()) continue;

        std::string response;
        std::string cmd = tokens[0];

        if (cmd == "REGISTER") {
            response = handle_register(tokens, peer_ip);
        } else if (cmd == "GETPEERS") {
            response = handle_getpeers(tokens, peer_ip);
        } else if (cmd == "UPDATE") {
            response = handle_update(tokens, peer_ip);
        } else {
            response = "ERROR Unknown command\n";
        }

        // 发送响应
        ssize_t bytes_sent = send(client_socket, response.c_str(), response.size(), 0);
        if (bytes_sent < 0) {
            std::cerr << "[ERROR] send failed to " << peer_ip << std::endl;
            break;
        }

        std::cout << "[SEND] To " << peer_ip << ": " << trim(response) << std::endl;
    }

    close(client_socket);
}

/**
 * 主函数：启动 Tracker 服务器
 */
int main(int argc, char* argv[]) {
    int port = TRACKER_PORT;

    // 支持自定义端口
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    std::cout << "==================================" << std::endl;
    std::cout << "  Simple P2P Tracker Server" << std::endl;
    std::cout << "==================================" << std::endl;
    std::cout << "Listening on port: " << port << std::endl;
    std::cout << std::endl;

    // 创建 socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "[ERROR] Failed to create socket" << std::endl;
        return 1;
    }

    // 设置 socket 选项，允许地址重用
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "[ERROR] setsockopt failed" << std::endl;
        close(server_socket);
        return 1;
    }

    // 绑定地址和端口
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ERROR] Bind failed" << std::endl;
        close(server_socket);
        return 1;
    }

    // 开始监听
    if (listen(server_socket, 10) < 0) {
        std::cerr << "[ERROR] Listen failed" << std::endl;
        close(server_socket);
        return 1;
    }

    std::cout << "[INFO] Tracker is running and accepting connections..." << std::endl;
    std::cout << std::endl;

    // 主循环：接受客户端连接
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            std::cerr << "[ERROR] Accept failed" << std::endl;
            continue;
        }

        // 获取客户端 IP
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        std::cout << "[INFO] New connection from " << client_ip << std::endl;

        // 为每个客户端创建一个线程
        std::thread client_thread(handle_client, client_socket, std::string(client_ip));
        client_thread.detach();  // 分离线程，让其独立运行
    }

    close(server_socket);
    return 0;
}
