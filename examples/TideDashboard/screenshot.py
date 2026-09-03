#!/usr/bin/env python3
# Grabs a PNG screenshot of the Tab5's current screen over serial/USB-CDC.
# Requires the firmware's screenshot dump (TideDashboard.ino: captureScreenshot(),
# triggered by sending the byte 'S' — see loop()).
#
# Usage: python3 screenshot.py [output.png]
# Defaults to screenshots/<timestamp>.png in this directory.

import datetime
import os
import serial
import struct
import sys
import time
from PIL import Image

PORT = "/dev/ttyACM0"
BAUD = 115200
MAGIC = b"SSHT"


def main():
    if len(sys.argv) > 1:
        out_path = sys.argv[1]
    else:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        ts = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
        out_path = os.path.join(script_dir, "screenshots", f"{ts}.png")
    os.makedirs(os.path.dirname(out_path) or ".", exist_ok=True)

    ser = serial.Serial(PORT, BAUD, timeout=5)
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.write(b"S")

    # Scan the (log-line-polluted) stream for the magic header.
    window = b""
    deadline = time.time() + 10
    while time.time() < deadline:
        chunk = ser.read(1)
        if not chunk:
            continue
        window = (window + chunk)[-4:]
        if window == MAGIC:
            break
    else:
        print("Timed out waiting for screenshot header")
        sys.exit(1)

    header = ser.read(4)
    w, h = struct.unpack("<HH", header)
    print(f"Receiving {w}x{h} frame ({w * h * 2} bytes)...")

    total = w * h * 2
    data = bytearray()
    while len(data) < total:
        chunk = ser.read(total - len(data))
        if not chunk:
            print(f"Stalled after {len(data)}/{total} bytes")
            sys.exit(1)
        data += chunk

    img = Image.new("RGB", (w, h))
    px = img.load()
    for y in range(h):
        row_off = y * w * 2
        for x in range(w):
            # Big-endian: M5GFX stores 16bpp pixels MSB-first (SPI TFT panel convention).
            v = (data[row_off + x * 2] << 8) | data[row_off + x * 2 + 1]
            r5 = (v >> 11) & 0x1F
            g6 = (v >> 5) & 0x3F
            b5 = v & 0x1F
            r = (r5 * 255) // 31
            g = (g6 * 255) // 63
            b = (b5 * 255) // 31
            px[x, y] = (r, g, b)

    img.save(out_path)
    print(f"Saved {out_path}")


if __name__ == "__main__":
    main()
