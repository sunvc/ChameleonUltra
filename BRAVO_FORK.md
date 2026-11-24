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
| 6042-6045 | License/secret/trial storage, no gating (trial always unlimited) |
| 6046-6049 | IC (HF) / ID (LF) field-wake polling gates |
| 6050-6053 | Custom git, sleep time, fence key |
| 6060 / 6061 | Permanent awake get/set |
| 6062 / 6063 | Smart-poll sector filter get/set |
| 6076 / 6077 | BLE GAP name get/set |
| 1763 / 6001 | Slot poll list (1763 read, 6001 write 32-byte) |
| 8001 | Alias of the MF1 block write for 32-slot builds |
| 4009 | MF1 emulator config — returns **6** fields (upstream 5 + PRNG) |
| 6090 / 6091 | Capture buffer read / clear (protocol analysis) |

Fence commands (6064/6067/6068/6069/6071/6072/6073/6074) are documented in
the section below.

Client compatibility notes (verified against GameTec-live/ChameleonUltraGUI):

- 6001 is only treated as the slot poll list when the payload is 32 bytes;
  other lengths fall through to the upstream HF14A-4 APDU send, so APDU
  tooling keeps working.
- 1019 / 1023 return 32 slots (128 / 64 bytes). Clients that hardcode 8
  slots (the GUI) read the first 8 and ignore the rest — no crash, but
  slots 8-31 are not visible there.
- 4009's extra 6th byte is ignored by clients that read only the first 5.

## Fence protocol (dalong companion app)

Reconstructed from real BLE captures of the "无感刷卡助手" bundle
(`cn.dalong.geofence`, Flutter). The app uploads a card stored in the app
into a device slot and activates it on a geofence event. Full working
sequence, in order:

| Cmd | Payload | Meaning | Impl |
|---|---|---|---|
| 6069 | empty | fence status probe → reply `SUCCESS` + `01` | `bravo_cmd_check_fence_status` |
| 6074 | empty | pre-check | ack |
| 6072 | 5 `[slot, sense, enable, tag_type BE16]` | set fence card type | `bravo_cmd_set_fence_card` → `tag_emulation_change_type` |
| 6071 | 2 `[tag_type BE16]` | confirm | ack |
| 6064 | `uidlen\|uid\|atqa\|sak\|atslen\|ats` | anti-collision | `bravo_cmd_fence_anticoll` → upstream **4001** handler |
| 6067 | `[block]` + N×16-byte blocks | MF1 block write | `bravo_cmd_fence_block_write` → upstream **4000** handler |
| 6073 | 4 bytes | fence config | ack |
| 1007 | `[slot, sense, nick]` | slot nick (standard) | upstream |
| 1006 | `[slot, sense, enable]` | slot enable (standard) | upstream |
| 6068 | 1 `[slot]` | activate slot + LED marquee | `bravo_cmd_set_fence_active_slot` → `tag_emulation_change_slot` + `apply_slot_change` |

Notes:

- 6069 with a 2-byte `[subcmd BE16]` still runs that sub-command on the
  active slot (legacy forwarder form).
- 6068 also accepts the 3-byte `[slot, subcmd BE16]` forwarder form.
- 6064 and 6067 reuse upstream handlers because the wire format matches
  4001 / 4000 exactly.
- The app probes fence support with 6069; answering `PAR_ERR` makes it
  report "fence firmware unavailable" and stop. Zero-length 6069 must
  return `SUCCESS`.
- Settings page "加载设置" reads 4009 and requires the 6-byte reply; the
  upstream 5-byte reply made it fail. Byte 5 is PRNG type.

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
