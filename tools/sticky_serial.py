"""Local display diagnostics for the connected Sticky; no wireless test actions."""
import argparse
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument("mode", choices=["listen", "status", "dump", "clean", "test", "tick", "menu", "preview", "powertest", "poweroff"])
parser.add_argument("--view", choices=["deauth", "recon"], default="deauth")
parser.add_argument("--port", required=True)
parser.add_argument("--seconds", type=float, default=12)
parser.add_argument("--output", default="sticky-frame.pbm")
parser.add_argument("--log", help="Save serial text to this UTF-8 file")
args = parser.parse_args()
port = serial.Serial()
port.port = args.port
port.baudrate = 115200
port.timeout = 0.25
port.dtr = False
port.rts = False
port.open()
log = None
try:
    if args.log:
        log = Path(args.log).open("w", encoding="utf-8")
    if args.mode != "listen":
        port.reset_input_buffer()
        command = "sticky " + args.mode
        if args.mode == "preview":
            command += " " + args.view
        port.write((command + "\n").encode())
    deadline = time.monotonic() + args.seconds
    while time.monotonic() < deadline:
        line = port.readline()
        if line.startswith(b"STICKY_FRAME_BEGIN"):
            fields = line.split()
            width, height, count = map(int, fields[1:])
            if (width, height, count) != (800, 480, 48000):
                raise RuntimeError("Unexpected frame dimensions")
            data = bytearray()
            port.timeout = 2
            while len(data) < count:
                chunk = port.read(count - len(data))
                if not chunk:
                    raise RuntimeError("Truncated framebuffer")
                data.extend(chunk)
            # PBM uses 1=black; controller RAM uses 1=white.
            pixels = bytes(value ^ 255 for value in data)
            Path(args.output).write_bytes(b"P4\n800 480\n" + pixels)
            print(f"Saved {args.output}")
            break
        if line:
            text = line.decode("utf-8", errors="replace")
            print(text, end="", flush=True)
            if log:
                log.write(text)
                log.flush()
finally:
    if log:
        log.close()
    port.close()
