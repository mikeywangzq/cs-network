# Makefile for DNS Resolver
# 简化编译过程

# 编译器设置
CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -O2

# 目标文件
TARGET = resolver

# 源文件
SOURCES = resolver.cpp

# 默认目标
all: $(TARGET)

# 编译规则
$(TARGET): $(SOURCES)
	@echo "正在编译 DNS 解析器..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCES)
	@echo "编译完成! 可执行文件: $(TARGET)"
	@echo ""
	@echo "使用方法:"
	@echo "  ./$(TARGET) google.com"
	@echo "  ./$(TARGET) www.baidu.com"

# 清理编译产物
clean:
	@echo "清理编译文件..."
	rm -f $(TARGET)
	@echo "清理完成!"

# 运行测试
test: $(TARGET)
	@echo "运行测试..."
	@echo ""
	@echo "测试 1: 查询 google.com"
	./$(TARGET) google.com
	@echo ""
	@echo "测试 2: 查询 www.baidu.com"
	./$(TARGET) www.baidu.com

# 帮助信息
help:
	@echo "DNS 解析器 Makefile"
	@echo ""
	@echo "可用目标:"
	@echo "  make       - 编译程序 (默认)"
	@echo "  make all   - 编译程序"
	@echo "  make clean - 清理编译文件"
	@echo "  make test  - 编译并运行测试"
	@echo "  make help  - 显示此帮助信息"

.PHONY: all clean test help
