#!/usr/bin/env python3
"""
ESP32 Marauder — Live Wireshark Streamer (Radiotap & Headless Edition)
Pipes live PCAP/Radiotap captures from ESP32 Marauder directly into Wireshark or a Named Pipe (FIFO).

Usage:
    python3 wireshark_live.py <serial_port> [baud_rate] [--pipe <fifo_path>] [--cmd <scan_cmd>]

Examples:
    python3 wireshark_live.py /dev/ttyUSB0
    python3 wireshark_live.py /dev/tty.usbserial-0001 921600
    python3 wireshark_live.py COM3 921600
    python3 wireshark_live.py /dev/ttyUSB0 921600 --cmd sniffbeacon
    python3 wireshark_live.py /dev/ttyUSB0 921600 --pipe /tmp/marauder.pcap
"""

import sys
import os
import subprocess
import time
import argparse

try:
    import serial
except ImportError:
    print("[!] 'pyserial' is required. Install it using:")
    print("    pip install pyserial")
    sys.exit(1)

BUF_BEGIN = b"[BUF/BEGIN]"
BUF_CLOSE = b"[BUF/CLOSE]"


def parse_arguments():
    parser = argparse.ArgumentParser(
        description="Stream live PCAP with Radiotap from ESP32 Marauder to Wireshark or Named Pipe."
    )
    parser.add_argument("port", help="Serial port of ESP32 (e.g. /dev/ttyUSB0, COM3)")
    parser.add_argument("baud", nargs="?", type=int, default=921600, help="Baud rate (default: 921600)")
    parser.add_argument("--pipe", default=None, help="Optional Named Pipe (FIFO) path to write PCAP data into")
    parser.add_argument("--cmd", default="sniffraw", help="Initial sniffing command to send (default: sniffraw)")
    return parser.parse_args()


def main():
    args = parse_arguments()
    port = args.port
    baud = args.baud
    pipe_path = args.pipe
    sniff_cmd = args.cmd

    print(f"[*] Opening serial port {port} at {baud} baud...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except Exception as e:
        print(f"[!] Error opening serial port {port}: {e}")
        sys.exit(1)

    pipe_fd = None
    wireshark_proc = None

    if pipe_path:
        if not os.path.exists(pipe_path):
            try:
                os.mkfifo(pipe_path)
                print(f"[*] Created FIFO named pipe at: {pipe_path}")
            except AttributeError:
                print(f"[*] OS does not support mkfifo, opening file directly: {pipe_path}")
        print(f"[*] Open Wireshark with: wireshark -k -i {pipe_path}")
        print(f"[*] Waiting for connection to pipe {pipe_path}...")
        try:
            pipe_fd = open(pipe_path, "wb")
        except Exception as e:
            print(f"[!] Failed to open pipe: {e}")
            ser.close()
            sys.exit(1)
    else:
        print("[*] Launching Wireshark in live capture mode...")
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
            print("    Please install Wireshark or specify --pipe <fifo_path>.")
            ser.close()
            sys.exit(1)

    print(f"[*] Sending '{sniff_cmd}' command to ESP32 Marauder...")
    time.sleep(1.0)
    ser.reset_input_buffer()
    ser.write(f"{sniff_cmd}\n".encode("utf-8"))
    ser.flush()

    print("[*] Streaming live 802.11 Radiotap packets... (Press Ctrl+C to stop)")
    in_buffer = False
    raw_accum = bytearray()

    try:
        while True:
            if wireshark_proc and wireshark_proc.poll() is not None:
                print("[*] Wireshark closed by user.")
                break

            chunk = ser.read(4096)
            if not chunk:
                continue

            raw_accum.extend(chunk)

            while True:
                if not in_buffer:
                    begin_idx = raw_accum.find(BUF_BEGIN)
                    if begin_idx != -1:
                        # Discard everything before BUF_BEGIN
                        raw_accum = raw_accum[begin_idx + len(BUF_BEGIN):]
                        in_buffer = True
                    else:
                        # Check if raw_accum is receiving un-encapsulated raw PCAP directly
                        if len(raw_accum) >= 4 and raw_accum[:4] in (b"\xa1\xb2\xc3\xd4", b"\xd4\xc3\xb2\xa1"):
                            # Direct raw stream mode
                            data_to_write = bytes(raw_accum)
                            raw_accum.clear()
                            if pipe_fd:
                                pipe_fd.write(data_to_write)
                                pipe_fd.flush()
                            elif wireshark_proc and wireshark_proc.stdin:
                                wireshark_proc.stdin.write(data_to_write)
                                wireshark_proc.stdin.flush()
                        else:
                            # Keep only small tail in case BUF_BEGIN was split across chunks
                            if len(raw_accum) > len(BUF_BEGIN):
                                raw_accum = raw_accum[-len(BUF_BEGIN):]
                        break
                else:
                    close_idx = raw_accum.find(BUF_CLOSE)
                    if close_idx != -1:
                        pcap_data = bytes(raw_accum[:close_idx])
                        raw_accum = raw_accum[close_idx + len(BUF_CLOSE):]
                        in_buffer = False

                        if pcap_data:
                            try:
                                if pipe_fd:
                                    pipe_fd.write(pcap_data)
                                    pipe_fd.flush()
                                elif wireshark_proc and wireshark_proc.stdin:
                                    wireshark_proc.stdin.write(pcap_data)
                                    wireshark_proc.stdin.flush()
                            except (BrokenPipeError, OSError):
                                break
                    else:
                        # Still inside buffer chunk, wait for more data
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

        if pipe_fd:
            try:
                pipe_fd.close()
            except Exception:
                pass

        if wireshark_proc and wireshark_proc.poll() is None:
            wireshark_proc.terminate()

        print("[*] Finished.")


if __name__ == "__main__":
    main()
