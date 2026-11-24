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

// Dalong-compatible per-slot poll list (cmds 1763/6001) and IC/ID gates
// (cmds 6046-6049). The list length is always TAG_MAX_SLOT_NUM.
void bravo_polling_get_slot_list(uint8_t *out);
bool bravo_polling_set_slot_list(const uint8_t *in, uint16_t length);
bool bravo_polling_get_sense_gate(bool hf);
void bravo_polling_set_sense_gate(bool hf, bool enable);
// Smart-poll sector filter switch (cmds 6062/6063)
bool bravo_polling_get_smart(void);
void bravo_polling_set_smart(bool enable);

// BLE command processors (cmds 6038-6041).
data_frame_tx_t *bravo_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_smart_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_smart_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
