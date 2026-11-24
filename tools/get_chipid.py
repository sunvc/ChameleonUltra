#!/usr/bin/env python3
"""Read the Chameleon device chip ID (cmd 1011) and firmware version (6050)
over USB CDC, and print the JSON body for /ultra/api/v1/device/register.

Usage:
    python tools/get_chipid.py [port]
    python tools/get_chipid.py [port] --register <12位激活码>

Requires pyserial (use cu-venv python).
"""
import glob
import json
import sys
import time

import serial

BAUD = 115200
CMD_GET_DEVICE_CHIP_ID = 1011
CMD_GET_CUSTOM_GIT = 6050


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
            buf.clear()
            continue
        if len(buf) == 2 and buf[1] != lrc(buf[0:1]):
            buf.clear()
            continue
        if len(buf) >= 8:
            ln = int.from_bytes(buf[6:8], "big")
            if len(buf) == 8 + ln + 1:
                return bytes(buf)
    return None


def request(ser, cmd, data=b""):
    ser.reset_input_buffer()
    ser.write(make_frame(cmd, data))
    fr = read_frame(ser)
    if not fr:
        raise RuntimeError(f"cmd {cmd} no response")
    status = int.from_bytes(fr[4:6], "big")
    ln = int.from_bytes(fr[6:8], "big")
    return status, fr[9:9 + ln]


def main():
    args = [a for a in sys.argv[1:]]
    code = None
    if "--register" in args:
        i = args.index("--register")
        code = args[i + 1]
        del args[i:i + 2]
    port = args[0] if args else find_port()
    if not port:
        print("no serial port")
        return 1

    ser = serial.Serial(port, BAUD, timeout=0.2)
    time.sleep(0.2)

    st, chip = request(ser, CMD_GET_DEVICE_CHIP_ID)
    if st != 0x68:
        print(f"chip id failed: status=0x{st:02x}")
    chip_id = chip.hex().upper()

    st2, ver = request(ser, CMD_GET_CUSTOM_GIT)
    firmware = ver.decode("utf-8", "replace") if st2 == 0x68 else ""
    ser.close()

    print(f"port:      {port}")
    print(f"chip_id:   {chip_id}")
    print(f"firmware:  {firmware}")
    print()
    body = {"activation_code": code or "<12位激活码>",
            "chip_id": chip_id,
            "firmware_version": firmware}
    print("POST https://ultra.tlq520.cn/ultra/api/v1/device/register")
    print("Content-Type: application/json")
    print(json.dumps(body, ensure_ascii=False))
    return 0


if __name__ == "__main__":
    sys.exit(main())
