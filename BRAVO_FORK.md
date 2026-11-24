# BRAVO fork layout

Thin fork of RfidResearchGroup/ChameleonUltra. All custom code lives in
`firmware/application/src/bravo/`; upstream files contain only single-line hook
points marked `// BRAVO fork hook`, so upstream merges conflict only at those
lines.

## Syncing upstream

```bash
git fetch upstream
git switch main && git merge --ff-only upstream/main
git switch feature/bravo-custom && git merge main
```

Conflicts can only occur at the 18 marked hook lines plus the
`Makefile.defs` tag fallback and the `syssleep.h` delay value. If a hook line
disappears in an upstream refactor, re-add the same single call.

## Modules

| File | Responsibility |
|---|---|
| `bravo/bravo_ble_guard.c` | BLE disconnect policy: re-advertise only when the device stays awake |
| `bravo/bravo_polling.c` | dalong-compatible field-wake auto polling + MF1 smart-poll sector filtering |
| `bravo/mf1_offline_clone.c` | Blocking offline dictionary-based Mifare Classic clone engine |
| `bravo/bravo_offline_clone_job.c` | Non-blocking request/tick wrapper, LED result indication |
| `bravo/bravo_commands.h` / `.inc` | Fork BLE command IDs and the only command-table hook |
| `bravo/bravo_features.mk` | Fork source list; the only Makefile hook |

## Hook map (upstream files)

- `firmware/Makefile.defs` — git-describe fallback for tagless clones
- `firmware/application/Makefile` — one `-include bravo/bravo_features.mk`
- `app_main.c` — polling init, HF/LF wake, button, sleep hooks; clone tick
- `app_cmd.c` — one include + one `.inc` inside `m_data_cmd_map`
- `ble_main.c` — disconnect hook
- `nfc_mf1.c` — MF1 auth block/result hooks
- `utils/syssleep.h` — BLE disconnect sleep delay 4000 ms -> 1 ms

## BLE commands

| Cmd | Feature |
|---|---|
| 2023 / 2024 | Offline clone request / status (Ultra build only) |
| 6038 / 6039 | Auto polling get/set |
| 6040 / 6041 | Auto polling switch delay get/set |

## Deliberately not ported

- Extra-long press button feature and its commands 1049-1052 from the previous
  Max tree, per product decision.
- dalong trial/activation paywall (`Using trial...`, `Trial limit reached`).
- 255-slot adaptive slot system from the previous Max tree; this fork keeps
  the upstream 8-slot storage model. Revisit if a product requirement appears.
- Smart-poll details that cannot be recovered from binary strings alone are
  marked as inferred in `docs/bravo-polling-spec.md`.

## Build and flash

```bash
./tools/build_flash.sh
```

Both `CURRENT_DEVICE_TYPE=ultra` and `lite` build clean. Run `make clean`
when switching device type: the upstream Makefile does not track the flag
change, so an incremental switch silently links stale objects. DFU packaging
uses the repo key at `resource/dfu_key/chameleon.pem`; do not modify that
directory.
