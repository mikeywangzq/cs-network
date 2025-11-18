# Makefile for Simple P2P File Sharing Tool
# 编译 Tracker 服务器和 Peer 客户端

CXX = g++
CXXFLAGS = -std=c++11 -Wall -Wextra -pthread -O2
LDFLAGS = -pthread

# 目标程序
TRACKER = tracker
PEER = peer

# 源文件
TRACKER_SRC = tracker.cpp
PEER_SRC = peer.cpp

# 默认目标：编译所有程序
all: $(TRACKER) $(PEER)

# 编译 Tracker 服务器
$(TRACKER): $(TRACKER_SRC)
	@echo "Compiling Tracker Server..."
	$(CXX) $(CXXFLAGS) -o $(TRACKER) $(TRACKER_SRC) $(LDFLAGS)
	@echo "✓ Tracker compiled successfully"

# 编译 Peer 客户端
$(PEER): $(PEER_SRC)
	@echo "Compiling Peer Client..."
	$(CXX) $(CXXFLAGS) -o $(PEER) $(PEER_SRC) $(LDFLAGS)
	@echo "✓ Peer compiled successfully"

# 清理编译产物
clean:
	@echo "Cleaning up..."
	rm -f $(TRACKER) $(PEER)
	rm -f *.o
	@echo "✓ Clean complete"

# 运行 Tracker（默认端口 6881）
run-tracker:
	@echo "Starting Tracker on port 6881..."
	./$(TRACKER)

# 帮助信息
help:
	@echo "Simple P2P File Sharing - Build System"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build both tracker and peer (default)"
	@echo "  tracker      - Build tracker server only"
	@echo "  peer         - Build peer client only"
	@echo "  clean        - Remove compiled binaries"
	@echo "  run-tracker  - Run the tracker server"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Usage examples:"
	@echo "  make              # Compile everything"
	@echo "  make tracker      # Compile tracker only"
	@echo "  make clean        # Clean all binaries"
	@echo "  make run-tracker  # Start tracker server"

.PHONY: all clean run-tracker help
