# ===================================================================
# Makefile for my_ping.cpp
# ===================================================================

# 编译器
CXX = g++

# 编译选项
CXXFLAGS = -Wall -Wextra -O2

# 目标程序
TARGET = my_ping

# 源文件
SOURCES = my_ping.cpp

# ===================================================================
# 编译规则
# ===================================================================

all: $(TARGET)
	@echo "============================================"
	@echo "编译完成！"
	@echo "使用方法: sudo ./$(TARGET) <主机名或IP>"
	@echo "例如: sudo ./$(TARGET) google.com"
	@echo "============================================"

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# 清理编译产物
clean:
	rm -f $(TARGET)
	@echo "清理完成"

# 运行测试（ping google.com）
test: $(TARGET)
	@echo "测试Ping google.com (需要sudo权限)..."
	sudo ./$(TARGET) google.com

# 帮助信息
help:
	@echo "可用的make命令："
	@echo "  make        - 编译程序"
	@echo "  make clean  - 清理编译产物"
	@echo "  make test   - 编译并测试（ping google.com）"
	@echo "  make help   - 显示此帮助信息"

.PHONY: all clean test help
