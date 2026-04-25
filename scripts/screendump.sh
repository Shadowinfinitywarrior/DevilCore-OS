#!/bin/bash
# Boot QEMU with a monitor on port 4444
qemu-system-i386 -cdrom DevilCoreOS.iso -m 256M -vga std -boot d -display none -monitor telnet:127.0.0.1:4444,server,nowait &
QEMU_PID=$!

echo "Waiting for OS to boot..."
sleep 20

echo "Sending screendump command to QEMU monitor..."
# Send screendump command via netcat
(echo "screendump /tmp/devilcore_screen.ppm"; sleep 2; echo "quit") | nc 127.0.0.1 4444

echo "Converting to PNG..."
ffmpeg -y -i /tmp/devilcore_screen.ppm /home/darkdevil404/.gemini/antigravity/brain/7624ed97-9ff5-4f4c-b660-7ff4df91215d/artifacts/devilcore_screen.png 2>/dev/null

echo "Killing QEMU..."
kill $QEMU_PID 2>/dev/null || true
wait $QEMU_PID 2>/dev/null || true

echo "Done. Screendump saved to artifacts."
