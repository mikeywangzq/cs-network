#!/bin/bash

# 简化版 P2P 文件分享工具 - 演示脚本
# 这个脚本展示如何使用 Tracker 和 Peer 程序

set -e

echo "========================================"
echo "  Simple P2P File Sharing - Demo"
echo "========================================"
echo ""

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 检查程序是否已编译
if [ ! -f "./tracker" ] || [ ! -f "./peer" ]; then
    echo -e "${YELLOW}Programs not found. Compiling...${NC}"
    make clean
    make all
    echo ""
fi

# 创建测试目录
TEST_DIR="./test_p2p"
mkdir -p "$TEST_DIR"

# 清理旧的测试文件
rm -f "$TEST_DIR"/*.dat
rm -f "$TEST_DIR"/*.log

echo -e "${BLUE}Creating test file...${NC}"
# 创建一个 1MB 的测试文件
dd if=/dev/urandom of="$TEST_DIR/original.dat" bs=1024 count=1024 2>/dev/null
FILE_SIZE=$(stat -c%s "$TEST_DIR/original.dat")
echo -e "${GREEN}✓ Test file created: $FILE_SIZE bytes${NC}"
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
echo "  Seed Port    : $SEED_PORT"
echo "  Peer 1 Port  : $PEER1_PORT"
echo "  Peer 2 Port  : $PEER2_PORT"
echo ""

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Step 1: Start Tracker Server${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "Run this command in a separate terminal:"
echo -e "${GREEN}./tracker $TRACKER_PORT${NC}"
echo ""
echo "Press Enter when Tracker is running..."
read

echo ""
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Step 2: Start Seed (has full file)${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "Run this command in a separate terminal:"
echo -e "${GREEN}./peer seed $FILE_ID $TEST_DIR/original.dat $FILE_SIZE $SEED_PORT $TRACKER_IP $TRACKER_PORT${NC}"
echo ""
echo "Press Enter when Seed is running..."
read

echo ""
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Step 3: Start Peer 1 (download)${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "Run this command in a separate terminal:"
echo -e "${GREEN}./peer download $FILE_ID $TEST_DIR/downloaded1.dat $FILE_SIZE $PEER1_PORT $TRACKER_IP $TRACKER_PORT${NC}"
echo ""
echo "This peer will download the file from the seed."
echo ""
echo "Press Enter to continue to optional step 4..."
read

echo ""
echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}  Step 4: Start Peer 2 (optional)${NC}"
echo -e "${YELLOW}========================================${NC}"
echo ""
echo "You can start another peer to test multi-peer downloading:"
echo -e "${GREEN}./peer download $FILE_ID $TEST_DIR/downloaded2.dat $FILE_SIZE $PEER2_PORT $TRACKER_IP $TRACKER_PORT${NC}"
echo ""
echo "This peer will download from both the seed and Peer 1."
echo ""

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}  Verification${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "After downloads complete, verify the files:"
echo ""
echo "# Compare checksums:"
echo "md5sum $TEST_DIR/original.dat"
echo "md5sum $TEST_DIR/downloaded1.dat"
echo "md5sum $TEST_DIR/downloaded2.dat"
echo ""
echo "# All checksums should match!"
echo ""

echo -e "${GREEN}Demo guide complete!${NC}"
echo ""
echo "Tips:"
echo "  - Use Ctrl+C to stop each program"
echo "  - Check the logs to see the P2P protocol in action"
echo "  - Try stopping and restarting peers to test resilience"
echo ""
