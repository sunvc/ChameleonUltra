#ifndef MAX_COMMANDS_H
#define MAX_COMMANDS_H

#include <stdint.h>
#include "app_cmd.h"

// MAX fork BLE command IDs. They use ranges the upstream enum does not
// occupy, so data_cmd.h itself stays untouched.

#define DATA_CMD_MAX_MF1_OFFLINE_CLONE        (2023)
#define DATA_CMD_MAX_MF1_OFFLINE_CLONE_STATUS (2024)

#if defined(PROJECT_CHAMELEON_ULTRA)
data_frame_tx_t *max_cmd_offline_clone(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_offline_clone_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
#endif

// Field-wake auto polling (6038-6041, companion-app compatible IDs).
data_frame_tx_t *max_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
