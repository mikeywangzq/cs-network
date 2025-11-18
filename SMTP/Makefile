# Makefile for SMTP Client
# 用于编译 SMTP 客户端程序

# 编译器
CXX = g++

# 编译选项
# -Wall: 显示所有警告
# -std=c++11: 使用 C++11 标准
# -O2: 优化级别 2
CXXFLAGS = -Wall -std=c++11 -O2

# 目标可执行文件名
TARGET = smtp_client

# 源文件
SOURCES = smtp_client.cpp

# 默认目标：编译程序
all: $(TARGET)
	@echo "========================================="
	@echo "编译完成！"
	@echo "运行方式: ./$(TARGET)"
	@echo "========================================="

# 编译规则
$(TARGET): $(SOURCES)
	@echo "正在编译 SMTP 客户端..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)

# 清理编译生成的文件
clean:
	@echo "清理编译文件..."
	rm -f $(TARGET)
	@echo "清理完成！"

# 运行程序
run: $(TARGET)
	@echo "运行 SMTP 客户端..."
	@echo ""
	./$(TARGET)

# 帮助信息
help:
	@echo "========================================="
	@echo "SMTP 客户端 Makefile 帮助"
	@echo "========================================="
	@echo "可用命令："
	@echo "  make        - 编译程序"
	@echo "  make clean  - 清理编译文件"
	@echo "  make run    - 编译并运行程序"
	@echo "  make help   - 显示此帮助信息"
	@echo "========================================="

# 声明伪目标（这些目标不是文件名）
.PHONY: all clean run help
