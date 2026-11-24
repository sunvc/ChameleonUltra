# MAX fork polling — behavior spec (dalong-compatible)

Source: firmware `v6.0-69-g053ccab-dirty` (nRF52 application, build id string
`v2.2.0-33-dalong`). Static evidence only — no runtime capture. Items marked
**inferred** are reconstructed from log strings and call-site context, not
observed execution.

## Evidence: dalong log strings

Auto polling:

- `WakeUp from HF` / `WakeUp from LF` / `WakeUp from rfid field`
- `Unknown wakeup source for auto polling`
- `Auto polling only HF slots` / `Auto polling only LF slots`
- `Auto polling switched to slot %d`
- `Auto polling: restoring original slot %d`
- `Auto polling disabled by button press`
- `Save auto polling config list failed.`
- `Save migrated auto polling config list failed.`

Smart poll (Mifare Classic adaptive):

- `Smart poll: auto_polling not enabled in settings`
- `Smart poll: m_is_auto_polling is false`
- `Smart poll: m_is_event_based_polling is false`
- `Smart poll: no valid next slot found`
- `Smart poll disabled, no valid auth sector`
- `Smart poll disabled, no valid auth sector in auto_poll_slots`
- `Smart poll disabled by setting` / `... by setting in auto_poll_slots`
- `Smart poll enabled, filtering by sector %d`
- `Smart poll: last_auth_sector=%d, invalid=%d, use_filter=%d`
- `auto_poll_slots: last_auth_sector=%d, invalid=%d, use_filter=%d`
- `Non-zero sector auth success, adaptive polling continues`

Trial gate (NOT ported — paywall, not a polling behavior):

- `Using trial for auto polling. Remaining trials: %d`
- `Trial limit reached for auto polling feature. Please activate your device.`

## Implemented behavior

### Auto polling (field-wake slot selection)

1. A field wake runs the existing upstream `check_wakeup_src()` HF/LF branch.
   The fork hook runs before tag emulation starts.
2. When polling is enabled and not runtime-disabled, the sense type is taken
   from the reset reason: NFC mask -> HF, LPCOMP/LF gpregret -> LF.
3. Candidate slots: poll-enabled slots whose emulation is enabled for that
   sense type (`is_slot_enabled`). Round-robin from the active slot.
   - Only-HF / only-LF candidate sets produce the corresponding log line.
   - No candidate: stay on the active slot (`no valid next slot` equivalent).
4. On switch: remember the pre-poll slot, call `tag_emulation_change_slot`,
   log `Auto polling switched to slot %d`.
5. On field loss / sleep entry: restore the remembered slot
   (`Auto polling: restoring original slot %d`). **inferred** restore point.
6. A button press while polling is armed latches runtime disable
   (`Auto polling disabled by button press`), RAM-only, cleared at reboot.
   **inferred** persistence (dalong may store it; no evidence either way).
7. Per-slot poll-enable list and global enable persist to FDS
   (`Save auto polling config list failed.` is the failure path).

### BLE interface (companion-app compatible IDs from the previous Max tree)

| Cmd | Direction | Payload |
|---|---|---|
| 6038 GET_DEVICE_AUTO_POLLING | dev->app | u8 enabled |
| 6039 SET_DEVICE_AUTO_POLLING | app->dev | u8 enabled |
| 6040 GET_DEVICE_AUTO_POLLING_DELAY | dev->app | u16 ms (big endian) |
| 6041 SET_DEVICE_AUTO_POLLING_DELAY | app->dev | u16 ms (big endian) |

### Smart poll (Mifare Classic sector filtering)

Implemented on top of the upstream MF1 auth log pipeline:

- `append_mf1_auth_log_step1` hook records the block of the in-flight auth;
  `append_mf1_auth_log_step3(success)` commits `block / 4` as the slot's
  `last_auth_sector` (persisted in the polling FDS record). Non-zero sector
  successes log `Non-zero sector auth success, adaptive polling continues`.
- On the next field wake, if slot sector memory converges on one non-zero
  sector, candidate selection is filtered to matching slots
  (`Smart poll enabled, filtering by sector %d`). No filtered candidate
  falls back to plain round-robin (`Smart poll: no valid next slot found`).
- **inferred**: dalong's exact filter state machine (`invalid`,
  `use_filter`, `m_is_event_based_polling`) is not visible in strings beyond
  their names; the convergence rule above is a reconstruction, not observed
  behavior. Config struct is versioned (v2) so a future correction migrates
  cleanly.
