#!/data/data/com.termux/files/usr/bin/bash
# One-time Termux setup for ESP32 Marauder AI client
# Run once: bash termux_setup.sh

set -e

echo "[marauder] Updating packages..."
pkg update -y

echo "[marauder] Installing Python and OpenSSL..."
pkg install -y python openssl

echo "[marauder] Installing Python dependencies..."
pip install --upgrade pip
pip install mcp pyserial "mcp-use>=0.1" "langchain-openai>=0.1"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo ""
echo "Setup complete!"
echo ""
echo "Before running, make sure:"
echo "  1. The Marauder Controller app is open on your phone"
echo "  2. The ESP32 is plugged in via OTG cable and connected in the app"
echo ""
echo "Then run:"
echo "  export VENICE_API_KEY=your_key_here"
echo "  python $SCRIPT_DIR/client.py"
echo ""
echo "The app bridges the ESP32 USB serial to localhost:5555 automatically."
echo "No root required."
