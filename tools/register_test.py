#!/usr/bin/env python3
"""Call the dalong activation endpoint /ultra/api/v1/device/register.

Business logic only (no device access). Mirrors the companion app:
POST JSON {activation_code, chip_id, firmware_version}; hosts are tried
in order, switching only on a network error.

Usage:
    python tools/register_test.py <激活码> [--chip-id HEX] [--firmware VER]
"""
import argparse
import json

import requests

HOSTS = ["i.tlq520.cn", "ultra.tlq520.cn", "geofence.192216.xyz"]
DEFAULT_CHIP_ID = "F7779EF5F39C0604"
DEFAULT_FIRMWARE = "71e10b5"


def register(host, code, chip_id, firmware):
    url = f"https://{host}/ultra/api/v1/device/register"
    body = {
        "activation_code": code,
        "chip_id": chip_id,
        "firmware_version": firmware,
    }
    r = requests.post(url, json=body, timeout=10,
                      headers={"Content-Type": "application/json"})
    r.raise_for_status()
    data = r.json() if r.content else {}
    top = data
    sub = top.get("data") or top.get("result") or top
    ok = bool(top.get("success")) or top.get("code") == 200 or sub.get("code") == 200
    return {
        "host": host,
        "http": r.status_code,
        "success": ok,
        "message": str(top.get("message") or sub.get("message") or ""),
        "secretKey": str(sub.get("secretKey") or sub.get("secret_key")
                         or top.get("secretKey") or top.get("secret_key") or ""),
        "addressKey": str(sub.get("addressKey") or sub.get("address_key")
                          or top.get("addressKey") or top.get("address_key") or ""),
        "raw": data,
    }


def main():
    p = argparse.ArgumentParser()
    p.add_argument("code", help="12-digit activation code")
    p.add_argument("--chip-id", default=DEFAULT_CHIP_ID)
    p.add_argument("--firmware", default=DEFAULT_FIRMWARE)
    p.add_argument("--host", default=None, help="force a single host")
    args = p.parse_args()

    hosts = [args.host] if args.host else HOSTS
    result = None
    for host in hosts:
        try:
            result = register(host, args.code, args.chip_id, args.firmware)
        except requests.RequestException as e:
            print(f"[{host}] network error: {e}")
            continue  # switch host only on network error
        break

    if result is None:
        print("all hosts failed (network)")
        return 1
    print(json.dumps(result, ensure_ascii=False, indent=2))
    return 0 if result["success"] else 2


if __name__ == "__main__":
    raise SystemExit(main())
