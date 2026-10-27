"""M5Paper display diagnostics. Commands are restricted to local hardware/UI checks."""
import argparse
from pathlib import Path
import time
import serial

parser = argparse.ArgumentParser()
parser.add_argument('mode', choices=['listen', 'status', 'dump', 'clean', 'test', 'tick', 'menu', 'powertest', 'preview recon', 'preview deauth'])
parser.add_argument('--port', required=True)
parser.add_argument('--seconds', type=float, default=10)
parser.add_argument('--output', default='paper-frame.pbm')
parser.add_argument('--reset', action='store_true', help='Reset the ESP32 before listening')
args = parser.parse_args()
port = serial.Serial()
port.port = args.port
port.baudrate = 115200
port.timeout = 0.25
port.dtr = False
port.rts = False
port.open()
try:
    if args.reset:
        port.rts = True
        time.sleep(0.15)
        port.rts = False
    if args.mode != 'listen':
        port.reset_input_buffer()
        port.write(('paper ' + args.mode + '\n').encode())
    deadline = time.monotonic() + args.seconds
    while time.monotonic() < deadline:
        line = port.readline()
        if line.startswith(b'PAPER_FRAME_BEGIN'):
            width, height, count = map(int, line.split()[1:])
            if (width, height, count) != (540, 960, 65280):
                raise RuntimeError('Unexpected framebuffer dimensions')
            data = bytearray()
            port.timeout = 2
            while len(data) < count:
                chunk = port.read(count - len(data))
                if not chunk:
                    raise RuntimeError('Truncated framebuffer')
                data.extend(chunk)
            Path(args.output).write_bytes(b'P4\n540 960\n' + bytes(v ^ 255 for v in data))
            print(f'Saved {args.output}')
            break
        if line:
            print(line.decode('utf-8', errors='replace'), end='', flush=True)
finally:
    port.close()
