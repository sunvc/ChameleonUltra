#!/usr/bin/env python3
"""Dalong 激活接口 /ultra/api/v1/device/register 的业务调用。

直接在 IDE 里点运行/调试即可，无需命令行参数。
改下面四个常量后运行。
"""
import json

import requests

# ───── 调试前改这里 ─────
ACTIVATION_CODE = "你的12位激活码"
CHIP_ID = "F7779EF5F39C0604"
FIRMWARE_VERSION = "71e10b5"
HOST = "i.tlq520.cn"        # 可选：ultra.tlq520.cn / geofence.192216.xyz
# ────────────────────────

URL = f"https://{HOST}/ultra/api/v1/device/register"

BODY = {
    "activation_code": ACTIVATION_CODE,
    "chip_id": CHIP_ID,
    "firmware_version": FIRMWARE_VERSION,
}

HEADERS = {"Content-Type": "application/json"}


def main():
    print("POST", URL)
    print("BODY", json.dumps(BODY, ensure_ascii=False))

    resp = requests.post(URL, json=BODY, headers=HEADERS, timeout=10)
    print("HTTP", resp.status_code)

    try:
        data = resp.json()
    except ValueError:
        print("非 JSON 响应:", resp.text)
        return

    top = data
    sub = top.get("data") or top.get("result") or top
    ok = bool(top.get("success")) or top.get("code") == 200 or sub.get("code") == 200
    result = {
        "success": ok,
        "message": top.get("message") or sub.get("message") or "",
        "secretKey": sub.get("secretKey") or sub.get("secret_key")
                     or top.get("secretKey") or top.get("secret_key") or "",
        "addressKey": sub.get("addressKey") or sub.get("address_key")
                      or top.get("addressKey") or top.get("address_key") or "",
    }
    print(json.dumps(result, ensure_ascii=False, indent=2))
    print("RAW", json.dumps(data, ensure_ascii=False))


if __name__ == "__main__":
    main()
