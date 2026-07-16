#!/usr/bin/env python3
"""Standalone PCAP diagnostic for the ESP32 Marauder Termux client.

Tells you *why* pcap capture is or isn't working, by connecting the same way
termux_client.py does, enabling SavePCAP, running `scanall -serial`, and checking
whether the firmware actually streams the binary PCAP markers over USB.

Run:
    cd ~/ESP32Marauder-cyd
    python mcp_server/pcap_diag.py [seconds]

No pip deps. Uses termux_client.py's connection backend, so it works over both the
direct-USB-serial (rooted) and Android-bridge paths.
"""
import os
import re
import socket
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import termux_client as tc   # noqa: E402


def main() -> None:
    secs = float(sys.argv[1]) if len(sys.argv) > 1 else 8.0

    print("=== Marauder PCAP diagnostic ===")
    print("Connecting…")
    print(tc._connect_bridge())
    if tc._sock is None:
        print("\nVERDICT: could not connect. Check the ESP32 (OTG), the Marauder "
              "Controller app / hotspot, or export MARAUDER_SERIAL=/dev/ttyUSB0.")
        return

    # 1. Is SavePCAP on? (capture buffers nothing without it)
    settings_txt = tc._send_cmd("settings", 5.0)
    m = re.search(r"SavePCAP[^\n]*", settings_txt, re.I)
    print("\nSavePCAP setting reported by device:",
          m.group(0).strip() if m else "(not found in `settings` output)")
    tc._send_cmd("settings -s SavePCAP enable", 3.0)

    # 2. Run scanall with -serial and watch the raw stream for PCAP markers.
    print(f"Running `scanall -serial` for {secs:.0f}s…")
    tc._drain()
    tc._sock.sendall(b"scanall -serial\n")
    demux = tc._PcapDemux()
    raw = bytearray()
    deadline = time.monotonic() + secs
    while time.monotonic() < deadline:
        try:
            chunk = tc._sock.recv(4096)
            if chunk:
                raw.extend(chunk)
                demux.feed(chunk)
        except socket.timeout:
            pass
    tc._send_cmd("stopscan -f", 5.0)

    markers = raw.count(b"[BUF/BEGIN]")
    print(f"\nReceived {len(raw):,} bytes, {markers} [BUF/BEGIN] marker(s), "
          f"{len(demux.pcap):,} pcap bytes demuxed.")
    print("First bytes (hex):", raw[:96].hex(" ") or "(none)")

    print()
    if markers or demux.pcap:
        print("VERDICT: firmware IS streaming PCAP over serial. Capture works — use")
        print("         /scan or /handshake then /save in termux_client.py to write .pcap.")
    elif raw:
        print("VERDICT: the device sent TEXT but ZERO [BUF/BEGIN] PCAP markers.")
        print("         Your FLASHED FIRMWARE predates serial-PCAP (the source supports it,")
        print("         your binary doesn't). Reflash the current CYD-no-screen build and")
        print("         retry. (If SavePCAP above showed 'false' and won't turn true, that")
        print("         is the other cause.)")
    else:
        print("VERDICT: no bytes received at all — a connection problem, not PCAP.")


if __name__ == "__main__":
    main()
