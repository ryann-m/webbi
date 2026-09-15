#!/bin/bash
set -u

SKETCH="arduino/webbi_display"
FQBN="esp32:esp32:esp32"
BAUD=115200

echo "🔌 Detecting ESP32 USB bus id via usbipd..."
# Match the USB-serial bridge (CP210x on your WROOM, or CH340/generic UART)
BUSID=$(powershell.exe -NoProfile -Command "usbipd list" 2>/dev/null \
  | grep -iE 'CP210|CH340|UART|Serial' \
  | head -n1 | awk '{print $1}' | tr -d '\r')

if [ -n "$BUSID" ]; then
  echo "✅ USB bridge on bus id: $BUSID"
  powershell.exe -NoProfile -Command "usbipd attach --wsl --busid $BUSID" >/dev/null 2>&1
else
  echo "⚠️  Auto-detect failed. Trying any already-attached device..."
  echo "   (One-time setup in an ADMIN PowerShell: usbipd bind --busid <id>)"
fi

echo "⏳ Waiting for serial port..."
PORT=""
for i in $(seq 1 10); do
  PORT=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null | head -n1)
  [ -n "$PORT" ] && break
  sleep 1
done

if [ -z "$PORT" ]; then
  echo "❌ ERROR: No ESP32 serial port (/dev/ttyUSB* or /dev/ttyACM*)."
  exit 1
fi
echo "✅ ESP32 on $PORT"

echo "⚙️  Compiling..."
arduino-cli compile --fqbn "$FQBN" "$SKETCH" || { echo "❌ Compile failed"; exit 1; }

read -p "Press Enter to upload..."
echo "🚀 Uploading..."
arduino-cli upload -p "$PORT" --fqbn "$FQBN" "$SKETCH" || { echo "❌ Upload failed"; exit 1; }

echo "🎉 SUCCESS. Monitor: arduino-cli monitor -p $PORT -c baudrate=$BAUD"