#ifndef BRAVO_COMMANDS_H
#define BRAVO_COMMANDS_H

#include <stdint.h>
#include "app_cmd.h"

// BRAVO fork BLE command IDs. They use ranges the upstream enum does not
// occupy, so data_cmd.h itself stays untouched.

#define DATA_CMD_BRAVO_MF1_OFFLINE_CLONE        (2023)
#define DATA_CMD_BRAVO_MF1_OFFLINE_CLONE_STATUS (2024)

#if defined(PROJECT_CHAMELEON_ULTRA)
data_frame_tx_t *bravo_cmd_offline_clone(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_offline_clone_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
#endif

// Field-wake auto polling (6038-6041, companion-app compatible IDs).
data_frame_tx_t *bravo_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

// Smart polling (6062/6063), implemented in bravo_polling.c
data_frame_tx_t *bravo_cmd_get_smart_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_set_smart_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

// Received-frame capture (6090/6091), implemented in bravo_capture.c
data_frame_tx_t *bravo_cmd_get_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_clear_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
