# BRAVO fork layout

Thin fork of RfidResearchGroup/ChameleonUltra. Feature code lives in
`firmware/application/src/bravo/`; feature entry into upstream files is
limited to single-line points marked `// BRAVO fork hook`. The fixed 32-slot
storage model necessarily edits the slot/LED subsystem directly — see the
hook map below for the full list.

## Syncing upstream

```bash
git fetch upstream
git switch main && git merge --ff-only upstream/main
git switch feature/bravo-custom && git merge main
```

Conflicts can only occur at the marked hook lines, the `Makefile.defs` tag
fallback, the `syssleep.h` delay value and the 32-slot storage/LED changes
listed below. If a hook line disappears in an upstream refactor, re-add the
same single call.

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

Storage-layer fork (32-slot model, not single-line hooks — upstream merges in
`tag_emulation.*` need review):

- `rfid/nfctag/tag_emulation.h` — `TAG_MAX_SLOT_NUM` 32, config v9 / 260 bytes
- `rfid/nfctag/tag_emulation.c` — v0..v8 -> v9 migration, 32-slot defaults
- `rfid/nfctag/tag_persistence.c` — FDS slot gate `slot >= TAG_MAX_SLOT_NUM`
- `app_cmd.c` — 1019/1023/1038 responses sized for 32 slots; plus the command-table include hook
- `rgb_marquee.c`, `rfid_main.c`, `app_main.c` — `% RGB_LIST_NUM` LED folding
- `utils/fds_ids.h` — slot file ID range comments

Single-line BRAVO hook points (the merge-sensitive surface for features):

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
- Dynamic add/remove slots: the fork uses a fixed 32-slot model (config
  struct v9, 260 bytes, FDS 0x1100/0x1200 + slot up to 0x111f/0x121f), not
  the old Max tree's 8..255 dynamic slot_count. v8/v0-7 records migrate on
  boot; slots 8-31 start disabled. Physical LED feedback folds slot indices
  modulo the 8 LEDs (`% RGB_LIST_NUM` guards at the animation entry points).
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
