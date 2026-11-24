#ifndef BRAVO_POLLING_H
#define BRAVO_POLLING_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// BRAVO fork: dalong-compatible field-wake auto polling.
// On an HF/LF field wake the module round-robins through slots that are
// both poll-enabled and emulation-enabled for the matching sense type,
// and restores the original slot when the device sleeps.

void bravo_polling_init(void);

// Wake/sleep/button hooks, each a single call at the upstream anchor.
// hf=true for an NFC/HF wake, false for an LF wake.
void bravo_polling_on_field_wake(bool hf);
void bravo_polling_on_button(void);
void bravo_polling_on_sleep(void);

// MF1 auth-event hooks for smart poll (called from nfc_mf1.c).
void bravo_polling_note_auth_block(uint8_t block);
void bravo_polling_note_auth_result(bool success);

// BLE command processors (cmds 6038-6041).
data_frame_tx_t *bravo_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
