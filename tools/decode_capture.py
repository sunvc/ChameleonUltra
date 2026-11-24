#!/usr/bin/env python3
"""Read and decode the BRAVO capture buffer (cmd 6090) over USB CDC.

Usage: python tools/decode_capture.py [port]
Prints each captured frame as: index  cmd  len  data(hex)
"""
import glob
import sys
import time

import serial

BAUD = 115200
CMD_GET_CAPTURE = 6090
ENTRY = 36  # 2 cmd + 2 len + 32 data


def find_port():
    for pat in ("/dev/cu.usbmodem*", "/dev/ttyACM*", "/dev/ttyUSB*"):
        ports = sorted(glob.glob(pat))
        if ports:
            return ports[0]
    return None


def lrc(b):
    return (0x100 - (sum(b) & 0xFF)) & 0xFF


def make_frame(cmd, data=b""):
    f = bytearray(9)
    f[0] = 0x11
    f[2:4] = cmd.to_bytes(2, "big")
    f[6:8] = len(data).to_bytes(2, "big")
    f[1] = lrc(f[0:1])
    f[8] = lrc(f[0:8])
    return bytes(f) + data + bytes([lrc(data)])


def read_frame(ser, timeout=3.0):
    buf = bytearray()
    end = time.time() + timeout
    while time.time() < end:
        c = ser.read(1)
        if not c:
            continue
        buf += c
        if len(buf) == 1 and buf[0] != 0x11:
            buf.clear(); continue
        if len(buf) == 2 and buf[1] != lrc(buf[0:1]):
            buf.clear(); continue
        if len(buf) >= 8:
            ln = int.from_bytes(buf[6:8], "big")
            if len(buf) == 8 + ln + 1:
                return buf
    return None


def main():
    port = sys.argv[1] if len(sys.argv) > 1 else find_port()
    if not port:
        print("no serial port"); return 1
    ser = serial.Serial(port, BAUD, timeout=0.2)
    time.sleep(0.2)
    ser.reset_input_buffer()
    ser.write(make_frame(CMD_GET_CAPTURE))
    fr = read_frame(ser)
    ser.close()
    if not fr:
        print("no response"); return 1
    status = int.from_bytes(fr[4:6], "big")
    ln = int.from_bytes(fr[6:8], "big")
    payload = fr[9:9 + ln]
    print(f"status=0x{status:02x} bytes={ln} frames={ln // ENTRY}")
    for i in range(0, ln - ENTRY + 1, ENTRY):
        cmd = int.from_bytes(payload[i:i+2], "big")
        flen = int.from_bytes(payload[i+2:i+4], "big")
        data = payload[i+4:i+4+min(flen, 32)]
        print(f"{i // ENTRY:3d}  cmd={cmd:5d}  len={flen:3d}  {data.hex()}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
