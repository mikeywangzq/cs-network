#!/bin/bash

# 自动化测试脚本 - 在后台运行所有组件并测试

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo "========================================"
echo "  Simple P2P - Automated Test"
echo "========================================"
echo ""

# 清理函数
cleanup() {
    echo ""
    echo -e "${YELLOW}Cleaning up...${NC}"
    # 杀死所有后台进程
    jobs -p | xargs -r kill 2>/dev/null || true
    sleep 1
    # 强制杀死（如果需要）
    pkill -f "./tracker" 2>/dev/null || true
    pkill -f "./peer" 2>/dev/null || true
    echo -e "${GREEN}✓ Cleanup complete${NC}"
}

# 设置陷阱以确保清理
trap cleanup EXIT INT TERM

# 检查并编译
if [ ! -f "./tracker" ] || [ ! -f "./peer" ]; then
    echo -e "${YELLOW}Compiling programs...${NC}"
    make clean
    make all
    echo ""
fi

# 创建测试目录
TEST_DIR="./test_p2p"
mkdir -p "$TEST_DIR"
rm -f "$TEST_DIR"/*.dat
rm -f "$TEST_DIR"/*.log

# 创建测试文件（256KB，这样下载会比较快）
echo -e "${BLUE}Creating test file (256KB)...${NC}"
dd if=/dev/urandom of="$TEST_DIR/original.dat" bs=1024 count=256 2>/dev/null
FILE_SIZE=$(stat -c%s "$TEST_DIR/original.dat")
echo -e "${GREEN}✓ Test file created: $FILE_SIZE bytes${NC}"
ORIGINAL_MD5=$(md5sum "$TEST_DIR/original.dat" | awk '{print $1}')
echo -e "${GREEN}✓ Original MD5: $ORIGINAL_MD5${NC}"
echo ""

# 测试参数
FILE_ID="testfile"
TRACKER_IP="127.0.0.1"
TRACKER_PORT=6881
SEED_PORT=7001
PEER1_PORT=7002
PEER2_PORT=7003

echo "Test Configuration:"
echo "  File ID      : $FILE_ID"
echo "  File Size    : $FILE_SIZE bytes"
echo "  Tracker      : $TRACKER_IP:$TRACKER_PORT"
echo ""

# 启动 Tracker
echo -e "${BLUE}[1/4] Starting Tracker...${NC}"
./tracker $TRACKER_PORT > "$TEST_DIR/tracker.log" 2>&1 &
TRACKER_PID=$!
sleep 2

if ! ps -p $TRACKER_PID > /dev/null; then
    echo -e "${RED}✗ Failed to start Tracker${NC}"
    cat "$TEST_DIR/tracker.log"
    exit 1
fi
echo -e "${GREEN}✓ Tracker started (PID: $TRACKER_PID)${NC}"
echo ""

# 启动 Seed
echo -e "${BLUE}[2/4] Starting Seed with complete file...${NC}"
./peer seed "$FILE_ID" "$TEST_DIR/original.dat" "$FILE_SIZE" $SEED_PORT $TRACKER_IP $TRACKER_PORT > "$TEST_DIR/seed.log" 2>&1 &
SEED_PID=$!
sleep 2

if ! ps -p $SEED_PID > /dev/null; then
    echo -e "${RED}✗ Failed to start Seed${NC}"
    cat "$TEST_DIR/seed.log"
    exit 1
fi
echo -e "${GREEN}✓ Seed started (PID: $SEED_PID)${NC}"
echo ""

# 启动 Peer 1 (下载器)
echo -e "${BLUE}[3/4] Starting Peer 1 (downloader)...${NC}"
./peer download "$FILE_ID" "$TEST_DIR/downloaded1.dat" "$FILE_SIZE" $PEER1_PORT $TRACKER_IP $TRACKER_PORT > "$TEST_DIR/peer1.log" 2>&1 &
PEER1_PID=$!
echo -e "${GREEN}✓ Peer 1 started (PID: $PEER1_PID)${NC}"
echo ""

# 等待 Peer 1 下载完成
echo -e "${YELLOW}Waiting for Peer 1 to complete download...${NC}"
for i in {1..60}; do
    if grep -q "Download completed" "$TEST_DIR/peer1.log" 2>/dev/null; then
        echo -e "${GREEN}✓ Peer 1 download complete!${NC}"
        break
    fi
    if [ $i -eq 60 ]; then
        echo -e "${RED}✗ Timeout waiting for Peer 1${NC}"
        echo "Last 20 lines of peer1.log:"
        tail -n 20 "$TEST_DIR/peer1.log"
        exit 1
    fi
    echo -n "."
    sleep 1
done
echo ""

# 验证 Peer 1 的文件
if [ -f "$TEST_DIR/downloaded1.dat" ]; then
    PEER1_MD5=$(md5sum "$TEST_DIR/downloaded1.dat" | awk '{print $1}')
    echo "Original MD5: $ORIGINAL_MD5"
    echo "Peer 1 MD5  : $PEER1_MD5"

    if [ "$ORIGINAL_MD5" = "$PEER1_MD5" ]; then
        echo -e "${GREEN}✓ Peer 1 file verified - checksums match!${NC}"
    else
        echo -e "${RED}✗ Peer 1 file verification failed - checksums don't match${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ Peer 1 downloaded file not found${NC}"
    exit 1
fi
echo ""

# 启动 Peer 2 (从 Seed 和 Peer 1 下载)
echo -e "${BLUE}[4/4] Starting Peer 2 (multi-source download)...${NC}"
sleep 2  # 让 Peer 1 完全转为 seed 模式
./peer download "$FILE_ID" "$TEST_DIR/downloaded2.dat" "$FILE_SIZE" $PEER2_PORT $TRACKER_IP $TRACKER_PORT > "$TEST_DIR/peer2.log" 2>&1 &
PEER2_PID=$!
echo -e "${GREEN}✓ Peer 2 started (PID: $PEER2_PID)${NC}"
echo ""

# 等待 Peer 2 下载完成
echo -e "${YELLOW}Waiting for Peer 2 to complete download...${NC}"
for i in {1..60}; do
    if grep -q "Download completed" "$TEST_DIR/peer2.log" 2>/dev/null; then
        echo -e "${GREEN}✓ Peer 2 download complete!${NC}"
        break
    fi
    if [ $i -eq 60 ]; then
        echo -e "${RED}✗ Timeout waiting for Peer 2${NC}"
        echo "Last 20 lines of peer2.log:"
        tail -n 20 "$TEST_DIR/peer2.log"
        exit 1
    fi
    echo -n "."
    sleep 1
done
echo ""

# 验证 Peer 2 的文件
if [ -f "$TEST_DIR/downloaded2.dat" ]; then
    PEER2_MD5=$(md5sum "$TEST_DIR/downloaded2.dat" | awk '{print $1}')
    echo "Original MD5: $ORIGINAL_MD5"
    echo "Peer 2 MD5  : $PEER2_MD5"

    if [ "$ORIGINAL_MD5" = "$PEER2_MD5" ]; then
        echo -e "${GREEN}✓ Peer 2 file verified - checksums match!${NC}"
    else
        echo -e "${RED}✗ Peer 2 file verification failed - checksums don't match${NC}"
        exit 1
    fi
else
    echo -e "${RED}✗ Peer 2 downloaded file not found${NC}"
    exit 1
fi
echo ""

# 测试成功
echo "========================================"
echo -e "${GREEN}  ✓ ALL TESTS PASSED!${NC}"
echo "========================================"
echo ""
echo "Summary:"
echo "  - Tracker: Running"
echo "  - Seed: Sharing complete file"
echo "  - Peer 1: Successfully downloaded from Seed"
echo "  - Peer 2: Successfully downloaded from multiple sources"
echo "  - All file checksums verified"
echo ""
echo "Logs are available in: $TEST_DIR/"
echo ""
echo "Press Ctrl+C to stop all processes and cleanup..."

# 保持运行以便查看日志
wait
