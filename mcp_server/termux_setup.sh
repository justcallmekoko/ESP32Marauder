#!/data/data/com.termux/files/usr/bin/bash
# One-time Termux setup for ESP32 Marauder AI client
# Run once after installing Termux:  bash termux_setup.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== ESP32 Marauder AI — Termux Setup ==="
echo ""

# ── 1. System packages ────────────────────────────────────────────────────────
echo "[1/5] Updating packages..."
pkg update -y && pkg upgrade -y

echo "[1/5] Installing Python, OpenSSL, clang..."
pkg install -y python openssl clang make

# ── 2. Python dependencies ────────────────────────────────────────────────────
echo "[2/5] Installing Python dependencies..."
pip install --upgrade pip
pip install mcp pyserial "mcp-use>=0.1" "langchain-openai>=0.1"

# ── 3. Android shared storage ─────────────────────────────────────────────────
echo "[3/5] Requesting storage permission..."
echo "      (A dialog will appear — tap ALLOW so captures save to your Downloads folder)"
termux-setup-storage
# Wait for the user to grant permission; the symlinks appear asynchronously
sleep 2
if [ -d "$HOME/storage/downloads" ]; then
    echo "      Storage permission granted. Captures will save to:"
    echo "      ~/storage/downloads/marauder_captures/"
    echo "      (visible in Android Files → Downloads → marauder_captures)"
else
    echo "      Storage permission not yet granted. Captures will save to:"
    echo "      ~/marauder_captures/ (Termux internal — not visible in Android Files)"
    echo "      Re-run termux-setup-storage manually to enable Android Files access."
fi

# ── 4. Venice AI API key ──────────────────────────────────────────────────────
echo ""
echo "[4/5] Venice AI API key setup"
if grep -q "VENICE_API_KEY" "$HOME/.bashrc" 2>/dev/null; then
    echo "      VENICE_API_KEY already set in ~/.bashrc — skipping."
else
    echo -n "      Enter your Venice AI API key (leave blank to skip): "
    read -r api_key
    if [ -n "$api_key" ]; then
        echo "" >> "$HOME/.bashrc"
        echo "# ESP32 Marauder AI" >> "$HOME/.bashrc"
        echo "export VENICE_API_KEY=\"$api_key\"" >> "$HOME/.bashrc"
        echo "      Saved to ~/.bashrc. It will be loaded in every new Termux session."
    else
        echo "      Skipped. Set it manually before running:"
        echo "      echo 'export VENICE_API_KEY=\"your_key\"' >> ~/.bashrc"
    fi
fi

# ── 5. Optional: port override ────────────────────────────────────────────────
echo ""
echo "[5/5] Port configuration"
echo "      The client auto-connects via socket://127.0.0.1:5555 on Termux."
echo "      No manual port configuration needed."
echo ""
if ! grep -q "MARAUDER_PORT" "$HOME/.bashrc" 2>/dev/null; then
    echo "      (If you ever need to override: export MARAUDER_PORT=socket://127.0.0.1:XXXX)"
fi

# ── Done ─────────────────────────────────────────────────────────────────────
echo ""
echo "=== Setup complete! ==="
echo ""
echo "Before running the client:"
echo "  1. Open the Marauder Controller app on your phone"
echo "  2. Plug the ESP32 into your phone via OTG cable"
echo "  3. Tap 'Connect' in the app — it will show 'Connected' when ready"
echo "     (The app bridges USB serial to localhost:5555 — no root needed)"
echo ""
echo "Then in a new Termux session:"
echo "  python $SCRIPT_DIR/client.py"
echo ""
echo "Captures save to:"
if [ -d "$HOME/storage/downloads" ]; then
    echo "  ~/storage/downloads/marauder_captures/"
    echo "  (open Android Files → Downloads → marauder_captures to browse them)"
else
    echo "  ~/marauder_captures/"
    echo "  (run termux-setup-storage and restart this script for Android Files access)"
fi
