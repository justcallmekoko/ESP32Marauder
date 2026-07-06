#!/usr/bin/env bash
# One-time Termux/proot-distro setup for ESP32 Marauder AI client
# Run once after installing Termux:  bash termux_setup.sh
#
# termux_client.py uses only Python stdlib — zero pip installs needed.
# The only requirements are Python itself and OpenSSL for HTTPS.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== ESP32 Marauder AI — Termux Setup ==="
echo ""

# ── 1. System packages ────────────────────────────────────────────────────────
if [ "$(id -u)" -ne 0 ] && command -v pkg &> /dev/null; then
    echo "[1/4] Updating packages via pkg..."
    pkg update -y
    echo "[1/4] Installing Python and OpenSSL via pkg..."
    pkg install -y python openssl
elif command -v apt-get &> /dev/null; then
    echo "[1/4] Updating packages via apt..."
    apt-get update -y
    echo "[1/4] Installing Python, venv, and OpenSSL via apt..."
    apt-get install -y python3 python3-venv python3-pip openssl
else
    echo "WARNING: No known package manager (pkg or apt) found. Skipping system package installation."
fi

# ── 2. Android shared storage ─────────────────────────────────────────────────
echo ""
echo "[2/4] Requesting storage permission..."
if command -v termux-setup-storage &> /dev/null; then
    echo "      (A dialog will appear — tap ALLOW so captures save to your Downloads folder)"
    termux-setup-storage || true
    sleep 2
else
    echo "      (Skipping termux-setup-storage as we are inside proot-distro)"
fi

if [ -d "$HOME/storage/downloads" ] || [ -d "/sdcard/Download" ]; then
    echo "      Storage permission/directory available."
else
    echo "      Storage path not found. Captures will save to:"
    echo "      ~/marauder_captures/"
fi

# ── 3. Venice AI API key ──────────────────────────────────────────────────────
echo ""
echo "[3/4] Venice AI API key setup"
if grep -q "VENICE_API_KEY" "$HOME/.bashrc" 2>/dev/null; then
    echo "      VENICE_API_KEY already set in ~/.bashrc — skipping."
else
    echo -n "      Enter your Venice AI API key (leave blank to skip): "
    read -r api_key || true
    if [ -n "$api_key" ]; then
        echo "" >> "$HOME/.bashrc"
        echo "# ESP32 Marauder AI" >> "$HOME/.bashrc"
        echo "export VENICE_API_KEY=\"$api_key\"" >> "$HOME/.bashrc"
        echo "      Saved to ~/.bashrc. Loaded in every new Termux session."
    else
        echo "      Skipped. Set it manually before running:"
        echo "      echo 'export VENICE_API_KEY=\"your_key\"' >> ~/.bashrc"
    fi
fi

# ── 4. Done ───────────────────────────────────────────────────────────────────
echo ""
echo "[4/4] Port configuration"
echo "      The client auto-connects via the Android app TCP bridge on localhost:7555."
echo "      No pip installs or manual port configuration needed."

echo ""
echo "=== Setup complete! ==="
echo ""
echo "Before running the client:"
echo "  1. Open the Marauder Controller app on your phone"
echo "  2. Plug the ESP32 into your phone via OTG cable"
echo "  3. Tap 'Connect' in the app — it will show 'Connected' when ready"
echo "     (The app bridges USB serial to localhost:7555 — no root needed)"
echo ""
echo "Then in a new Termux session:"
echo "  python $SCRIPT_DIR/termux_client.py"
echo ""
echo "Captures save to:"
if [ -d "$HOME/storage/downloads" ]; then
    echo "  ~/storage/downloads/marauder_captures/"
    echo "  (open Android Files → Downloads → marauder_captures to browse them)"
else
    echo "  ~/marauder_captures/"
    echo "  (run termux-setup-storage and restart this script for Android Files access)"
fi
