# ============================================================================
# Makefile for Epoll Chat Room
# ============================================================================

# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -std=c++11 -Wall -Wextra -O2 -pthread

# 目标可执行文件
SERVER = epoll_server
CLIENT = client

# 所有目标
all: $(SERVER) $(CLIENT)

# 编译服务器
$(SERVER): epoll_server.cpp
	@echo "正在编译服务器..."
	$(CXX) $(CXXFLAGS) -o $(SERVER) epoll_server.cpp
	@echo "服务器编译完成: $(SERVER)"

# 编译客户端
$(CLIENT): client.cpp
	@echo "正在编译客户端..."
	$(CXX) $(CXXFLAGS) -o $(CLIENT) client.cpp
	@echo "客户端编译完成: $(CLIENT)"

# 清理
clean:
	@echo "清理编译文件..."
	rm -f $(SERVER) $(CLIENT)
	@echo "清理完成"

# 运行服务器
run-server: $(SERVER)
	@echo "启动服务器..."
	./$(SERVER)

# 运行客户端
run-client: $(CLIENT)
	@echo "启动客户端..."
	./$(CLIENT)

# 帮助信息
help:
	@echo "可用的 make 命令:"
	@echo "  make          - 编译服务器和客户端"
	@echo "  make server   - 仅编译服务器"
	@echo "  make client   - 仅编译客户端"
	@echo "  make clean    - 清理编译文件"
	@echo "  make run-server - 编译并运行服务器"
	@echo "  make run-client - 编译并运行客户端"
	@echo "  make help     - 显示此帮助信息"

.PHONY: all clean run-server run-client help
