#ifndef MAX_OFFLINE_CLONE_JOB_H
#define MAX_OFFLINE_CLONE_JOB_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// MAX fork: host-triggered offline Mifare Classic clone.
// Request is non-blocking; max_offline_clone_tick() runs the job from the
// main loop. No button binding exists in this fork.

void max_offline_clone_request(void);
void max_offline_clone_tick(void);
bool max_offline_clone_is_running(void);

// Command processors for DATA_CMD_MAX_MF1_OFFLINE_CLONE(_STATUS).
data_frame_tx_t *max_cmd_offline_clone(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *max_cmd_offline_clone_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
