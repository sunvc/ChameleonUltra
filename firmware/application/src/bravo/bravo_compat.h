#ifndef BRAVO_COMPAT_H
#define BRAVO_COMPAT_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// BRAVO fork: compatibility with the dalong/Nolet companion-app 6000-range
// command set. Storage lives in a private FDS file; no paywall is enforced.

void bravo_compat_init(void);

// True while "disable sleep permanent" (cmd 6061) is on. The BLE disconnect
// hook consults this to keep the device awake and advertising.
bool bravo_compat_sleep_blocked(void);

// Early command intercept, run before the upstream command table.
// Returns a response when the cmd is owned by this module, NULL otherwise.
// Needed for 6001, which collides with upstream HF14A_4_APDU_SEND.
data_frame_tx_t *bravo_compat_intercept(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

// Table-registered command processors
data_frame_tx_t *bravo_cmd_get_license_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_license_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_secret_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_trial(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_ic_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_ic_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_id_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_id_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_custom_git(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_sleep_time(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_sleep_time(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_fence_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_disable_sleep(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_disable_sleep(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_ble_name(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_ble_name(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_slot_poll_list(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_fence_active_slot(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_check_fence_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
// Generic "accept and acknowledge" handler for fence commands whose exact
// semantics are still being mapped from app captures.
data_frame_tx_t *bravo_cmd_ack(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_fence_card(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
