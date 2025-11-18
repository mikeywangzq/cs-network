# Makefile for my_traceroute
# 简易 Traceroute 工具的编译脚本

# 编译器
CXX = g++

# 编译选项
# -Wall: 开启所有警告
# -Wextra: 开启额外警告
# -O2: 优化级别 2
CXXFLAGS = -Wall -Wextra -O2

# 目标可执行文件
TARGET = my_traceroute

# 源文件
SOURCES = my_traceroute.cpp

# 对象文件
OBJECTS = $(SOURCES:.cpp=.o)

# 默认目标
all: $(TARGET)
	@echo "✅ 编译完成！"
	@echo "💡 使用方法: sudo ./$(TARGET) <目标主机>"
	@echo "   示例: sudo ./$(TARGET) google.com"

# 链接生成可执行文件
$(TARGET): $(OBJECTS)
	@echo "🔗 链接 $(TARGET)..."
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJECTS)

# 编译源文件为对象文件
%.o: %.cpp
	@echo "🔨 编译 $<..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理编译生成的文件
clean:
	@echo "🧹 清理中..."
	rm -f $(OBJECTS) $(TARGET)
	@echo "✅ 清理完成！"

# 运行程序（需要提供目标主机）
# 用法: make run HOST=google.com
run: $(TARGET)
	@if [ -z "$(HOST)" ]; then \
		echo "❌ 错误：请指定目标主机"; \
		echo "   用法: make run HOST=<目标主机>"; \
		echo "   示例: make run HOST=google.com"; \
		exit 1; \
	fi
	@echo "🚀 运行 traceroute 到 $(HOST)..."
	sudo ./$(TARGET) $(HOST)

# 声明伪目标（这些目标不对应实际文件）
.PHONY: all clean run
