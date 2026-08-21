#!/usr/bin/env python3
"""
ESP32 Marauder — Live Wireshark Streamer
Pipe live PCAP captures from ESP32 Marauder (Headless CLI) directly into Wireshark.

Usage:
    python3 wireshark_live.py <serial_port> [baud_rate]

Examples:
    python3 wireshark_live.py /dev/ttyUSB0
    python3 wireshark_live.py /dev/tty.usbserial-0001 921600
    python3 wireshark_live.py COM3
"""

import sys
import subprocess
import time

try:
    import serial
except ImportError:
    print("[!] 'pyserial' is required. Install it using:")
    print("    pip install pyserial")
    sys.exit(1)


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 wireshark_live.py <serial_port> [baud_rate]")
        print("Example: python3 wireshark_live.py /dev/ttyUSB0 921600")
        sys.exit(1)

    port = sys.argv[1]
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else 921600

    print(f"[*] Opening serial port {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except Exception as e:
        print(f"[!] Error opening serial port {port}: {e}")
        sys.exit(1)

    # Launch Wireshark reading from stdin
    print("[*] Launching Wireshark...")
    wireshark_cmd = ["wireshark", "-k", "-i", "-"]
    try:
        wireshark_proc = subprocess.Popen(
            wireshark_cmd,
            stdin=subprocess.PIPE,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )
    except FileNotFoundError:
        print("[!] 'wireshark' executable not found in PATH.")
        print("    Please install Wireshark or make sure it's in your system PATH.")
        ser.close()
        sys.exit(1)

    print("[*] Wireshark started. Sending 'sniffraw' command to ESP32 Marauder...")
    time.sleep(1.0)
    ser.reset_input_buffer()
    ser.write(b"sniffraw\n")
    ser.flush()

    print("[*] Streaming live packets into Wireshark... (Press Ctrl+C to stop)")
    try:
        while wireshark_proc.poll() is None:
            data = ser.read(4096)
            if data:
                try:
                    wireshark_proc.stdin.write(data)
                    wireshark_proc.stdin.flush()
                except BrokenPipeError:
                    break
    except KeyboardInterrupt:
        print("\n[*] Stopping stream...")
    finally:
        print("[*] Sending 'stopscan' to ESP32...")
        try:
            ser.write(b"stopscan\n")
            ser.flush()
        except Exception:
            pass
        ser.close()
        if wireshark_proc.poll() is None:
            wireshark_proc.terminate()
        print("[*] Finished.")


if __name__ == "__main__":
    main()
