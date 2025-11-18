# Makefile for Simple FTP Server and Client

CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread
TARGET_SERVER = server
TARGET_CLIENT = client
SRC_SERVER = server.cpp
SRC_CLIENT = client.cpp

# 默认目标：编译服务器和客户端
all: $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "=========================================="
	@echo "编译完成！"
	@echo "运行服务器: ./server"
	@echo "运行客户端: ./client"
	@echo "=========================================="

# 编译服务器
$(TARGET_SERVER): $(SRC_SERVER)
	$(CXX) $(CXXFLAGS) -o $(TARGET_SERVER) $(SRC_SERVER)
	@echo "服务器编译完成: $(TARGET_SERVER)"

# 编译客户端
$(TARGET_CLIENT): $(SRC_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CLIENT) $(SRC_CLIENT)
	@echo "客户端编译完成: $(TARGET_CLIENT)"

# 清理编译产物
clean:
	rm -f $(TARGET_SERVER) $(TARGET_CLIENT)
	@echo "清理完成"

# 创建必要的目录
setup:
	mkdir -p server_files client_files
	@echo "目录创建完成: server_files/ 和 client_files/"

# 运行服务器
run-server: $(TARGET_SERVER)
	./$(TARGET_SERVER)

# 运行客户端
run-client: $(TARGET_CLIENT)
	./$(TARGET_CLIENT)

# 帮助信息
help:
	@echo "可用的 Make 命令："
	@echo "  make          - 编译服务器和客户端"
	@echo "  make server   - 只编译服务器"
	@echo "  make client   - 只编译客户端"
	@echo "  make clean    - 清理编译产物"
	@echo "  make setup    - 创建必要的目录"
	@echo "  make run-server - 编译并运行服务器"
	@echo "  make run-client - 编译并运行客户端"
	@echo "  make help     - 显示此帮助信息"

.PHONY: all clean setup run-server run-client help
