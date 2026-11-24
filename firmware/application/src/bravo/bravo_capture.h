#ifndef BRAVO_CAPTURE_H
#define BRAVO_CAPTURE_H

#include <stdint.h>
#include <stdbool.h>
#include "dataframe.h"

// BRAVO fork: RAM ring buffer of received BLE/USB command frames, for
// protocol analysis against the companion app. Read over the normal
// command channel (no debug probe needed).
//   6090 GET_CAPTURE  -> entries; each = cmd(BE16) len(BE16) data[32]
//   6091 CLEAR_CAPTURE

#define BRAVO_CAP_ENTRIES     64
#define BRAVO_CAP_DATA_LEN    32
#define BRAVO_CAP_ENTRY_SIZE  (2 + 2 + BRAVO_CAP_DATA_LEN)  // 36

void bravo_capture_push(uint16_t cmd, uint16_t length, const uint8_t *data);

data_frame_tx_t *bravo_cmd_get_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);
data_frame_tx_t *bravo_cmd_clear_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data);

#endif
