#!/bin/bash
# DevilCore OS - Test Script

cd /home/darkdevil404/DevilCore

echo "===== DevilCore OS Test ====="
echo ""
echo "Files created:"
ls -lh DevilCore*
echo ""

echo "Boot signature check:"
xxd -s 508 -l 4 build/boot.bin
echo ""

echo "Testing with QEMU..."
timeout 3 qemu-system-i386 -fda DevilCoreOS_floppy.img -m 64 2>&1 &
QEMU_PID=$!
sleep 2

if ps -p $QEMU_PID > /dev/null 2>&1; then
    echo "QEMU is running (PID: $QEMU_PID)"
    kill $QEMU_PID 2>/dev/null
    wait $QEMU_PID 2>/dev/null
    echo "Test complete"
else
    echo "QEMU failed to start"
fi

echo ""
echo "===== Done ====="