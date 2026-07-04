#!/data/data/com.termux/files/usr/bin/bash
# One-time Termux setup for ESP32 Marauder AI client
# Run once after installing Termux:  bash termux_setup.sh
#
# termux_client.py uses only Python stdlib — zero pip installs needed.
# The only requirements are Python itself and OpenSSL for HTTPS.

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== ESP32 Marauder AI — Termux Setup ==="
echo ""

# ── 1. System packages ────────────────────────────────────────────────────────
echo "[1/4] Updating packages..."
pkg update -y

echo "[1/4] Installing Python and OpenSSL..."
pkg install -y python openssl

# ── 2. Android shared storage ─────────────────────────────────────────────────
echo ""
echo "[2/4] Requesting storage permission..."
echo "      (A dialog will appear — tap ALLOW so captures save to your Downloads folder)"
termux-setup-storage
sleep 2
if [ -d "$HOME/storage/downloads" ]; then
    echo "      Storage permission granted. Captures will save to:"
    echo "      ~/storage/downloads/marauder_captures/"
else
    echo "      Storage permission not yet granted. Captures will save to:"
    echo "      ~/marauder_captures/ (Termux internal — not visible in Android Files)"
    echo "      Re-run: termux-setup-storage"
fi

# ── 3. Venice AI API key ──────────────────────────────────────────────────────
echo ""
echo "[3/4] Venice AI API key setup"
if grep -q "VENICE_API_KEY" "$HOME/.bashrc" 2>/dev/null; then
    echo "      VENICE_API_KEY already set in ~/.bashrc — skipping."
else
    echo -n "      Enter your Venice AI API key (leave blank to skip): "
    read -r api_key
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
echo "      The client auto-connects via the Android app TCP bridge on localhost:5555."
echo "      No pip installs or manual port configuration needed."

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
