#!/bin/bash
#
# XAE OS Test Script
# Tests core functionality to ensure changes don't break existing features
#

echo "==================================="
echo "XAE OS Functionality Test"
echo "==================================="

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

PASS=0
FAIL=0

# Helper functions
pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((PASS++))
}

fail() {
    echo -e "${RED}✗${NC} $1"
    ((FAIL++))
}

# Test 1: Build system
echo ""
echo "Test 1: Build System"
echo "-------------------"
if make clean > /dev/null 2>&1 && make > /dev/null 2>&1; then
    pass "Kernel builds successfully"
else
    fail "Kernel build failed"
fi

# Test 2: Check kernel size (should be under 100KB for floppy)
echo ""
echo "Test 2: Kernel Size"
echo "-------------------"
if [ -f build/kernel.bin ]; then
    SIZE=$(stat -f%z build/kernel.bin 2>/dev/null || stat -c%s build/kernel.bin 2>/dev/null)
    if [ $SIZE -lt 102400 ]; then
        pass "Kernel size OK ($SIZE bytes)"
    else
        fail "Kernel too large ($SIZE bytes, max 100KB)"
    fi
else
    fail "Kernel binary not found"
fi

# Test 3: Bootloader size (must be exactly 512 bytes)
echo ""
echo "Test 3: Bootloader Size"
echo "----------------------"
if [ -f build/boot.bin ]; then
    SIZE=$(stat -f%z build/boot.bin 2>/dev/null || stat -c%s build/boot.bin 2>/dev/null)
    if [ $SIZE -eq 512 ]; then
        pass "Bootloader is exactly 512 bytes"
    else
        fail "Bootloader is $SIZE bytes (must be 512)"
    fi
else
    fail "Bootloader binary not found"
fi

# Test 4: Check for required symbols in kernel
echo ""
echo "Test 4: Required Kernel Functions"
echo "---------------------------------"
REQUIRED_SYMBOLS=(
    "kernel_main"
    "shell_init"
    "shell_run"
    "xaefs_init"
    "serial_init"
)

for sym in "${REQUIRED_SYMBOLS[@]}"; do
    if nm build/kernel.bin 2>/dev/null | grep -q "$sym"; then
        pass "Symbol '$sym' found"
    else
        fail "Symbol '$sym' missing"
    fi
done

# Test 5: File structure
echo ""
echo "Test 5: Source File Structure"
echo "-----------------------------"
REQUIRED_FILES=(
    "boot/boot.asm"
    "kernel/kernel.c"
    "kernel/drivers/serial.c"
    "kernel/shell/shell.c"
    "kernel/fs/xaefs.c"
)

for file in "${REQUIRED_FILES[@]}"; do
    if [ -f "$file" ]; then
        pass "File '$file' exists"
    else
        fail "File '$file' missing"
    fi
done

# Test 6: QEMU boot test (quick boot and shutdown)
echo ""
echo "Test 6: QEMU Boot Test"
echo "---------------------"
timeout 5 qemu-system-i386 \
    -drive file=build/xae_os.img,format=raw,if=floppy \
    -boot a \
    -display none \
    -serial file:/tmp/xae_boot_test.log \
    > /dev/null 2>&1

if [ -f /tmp/xae_boot_test.log ]; then
    if grep -q "Ready" /tmp/xae_boot_test.log; then
        pass "OS boots and initializes serial"
    else
        fail "OS boots but serial initialization failed"
    fi
    rm -f /tmp/xae_boot_test.log
else
    fail "QEMU boot test failed"
fi

# Summary
echo ""
echo "==================================="
echo "Test Summary"
echo "==================================="
echo -e "${GREEN}Passed: $PASS${NC}"
echo -e "${RED}Failed: $FAIL${NC}"
echo "==================================="

if [ $FAIL -eq 0 ]; then
    echo -e "${GREEN}All tests passed!${NC}"
    exit 0
else
    echo -e "${RED}Some tests failed!${NC}"
    exit 1
fi
