# ============================================
# TCP 协议分析器 - Makefile
# ============================================

# 编译器配置
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2

# 目标文件
TARGET = tcp_analyzer

# 源文件
SOURCES = tcp_analyzer.cpp

# 对象文件
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标：编译程序
all: $(TARGET)
	@echo ""
	@echo "======================================================"
	@echo "  ✅ 编译成功！"
	@echo "======================================================"
	@echo "运行方式："
	@echo "  sudo ./$(TARGET) <网络接口名>"
	@echo ""
	@echo "示例："
	@echo "  sudo ./$(TARGET) eth0"
	@echo "  sudo ./$(TARGET) wlan0"
	@echo "  sudo ./$(TARGET) lo      # 本地回环接口"
	@echo "======================================================"
	@echo ""

# 编译规则
$(TARGET): $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# 编译 .cpp 文件为 .o 文件
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译产物
clean:
	rm -f $(OBJECTS) $(TARGET)
	@echo "✅ 清理完成"

# 运行程序（需要指定接口）
# 用法: make run INTERFACE=eth0
run: $(TARGET)
	@if [ -z "$(INTERFACE)" ]; then \
		echo "❌ 错误：请指定网络接口"; \
		echo "用法: make run INTERFACE=eth0"; \
		exit 1; \
	fi
	sudo ./$(TARGET) $(INTERFACE)

# 显示帮助信息
help:
	@echo "======================================================"
	@echo "  TCP 协议分析器 - Makefile 帮助"
	@echo "======================================================"
	@echo ""
	@echo "可用命令："
	@echo "  make              - 编译程序"
	@echo "  make clean        - 清理编译产物"
	@echo "  make run INTERFACE=<接口名> - 运行程序"
	@echo "  make help         - 显示此帮助信息"
	@echo ""
	@echo "示例："
	@echo "  make"
	@echo "  make run INTERFACE=eth0"
	@echo "  make clean"
	@echo "======================================================"
	@echo ""

.PHONY: all clean run help
