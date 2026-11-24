#ifndef MAX_POLLING_H
#define MAX_POLLING_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// MAX fork: dalong-compatible field-wake auto polling.
// On an HF/LF field wake the module round-robins through slots that are
// both poll-enabled and emulation-enabled for the matching sense type,
// and restores the original slot when the device sleeps.

void max_polling_init(void);

// Wake/sleep/button hooks, each a single call at the upstream anchor.
// hf=true for an NFC/HF wake, false for an LF wake.
void max_polling_on_field_wake(bool hf);
void max_polling_on_button(void);
void max_polling_on_sleep(void);

// MF1 auth-event hooks for smart poll (called from nfc_mf1.c).
void max_polling_note_auth_block(uint8_t block);
void max_polling_note_auth_result(bool success);

// BLE command processors (cmds 6038-6041).
data_frame_tx_t *max_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
