#ifndef BRAVO_OFFLINE_CLONE_JOB_H
#define BRAVO_OFFLINE_CLONE_JOB_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// BRAVO fork: host-triggered offline Mifare Classic clone.
// Request is non-blocking; bravo_offline_clone_tick() runs the job from the
// main loop. No button binding exists in this fork.

void bravo_offline_clone_request(void);
void bravo_offline_clone_tick(void);
bool bravo_offline_clone_is_running(void);

// Command processors for DATA_CMD_BRAVO_MF1_OFFLINE_CLONE(_STATUS).
data_frame_tx_t *bravo_cmd_offline_clone(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_offline_clone_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
