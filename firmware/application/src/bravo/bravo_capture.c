// BRAVO fork: received-command capture ring buffer.
// Frames are stored in RAM and read back over the normal command channel,
// so the companion app's protocol can be observed without a debug probe.

#include <string.h>

#include "app_status.h"
#include "bravo_capture.h"

static struct {
    uint16_t cmd;
    uint16_t len;
    uint8_t data[BRAVO_CAP_DATA_LEN];
} m_cap[BRAVO_CAP_ENTRIES];
static uint16_t m_head;   // next write index
static uint16_t m_count;  // filled entries (<= BRAVO_CAP_ENTRIES)

void bravo_capture_push(uint16_t cmd, uint16_t length, const uint8_t *data) {
    // Do not capture the capture-read/clear commands themselves
    if (cmd == 6090 || cmd == 6091) {
        return;
    }
    uint16_t i = m_head;
    m_cap[i].cmd = cmd;
    m_cap[i].len = length;
    uint16_t n = length > BRAVO_CAP_DATA_LEN ? BRAVO_CAP_DATA_LEN : length;
    if (n > 0 && data != NULL) {
        memcpy(m_cap[i].data, data, n);
    }
    m_head = (uint16_t)((m_head + 1) % BRAVO_CAP_ENTRIES);
    if (m_count < BRAVO_CAP_ENTRIES) {
        m_count++;
    }
}

data_frame_tx_t *bravo_cmd_get_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    // Build in a response buffer, oldest entry first
    static uint8_t out[BRAVO_CAP_ENTRIES * BRAVO_CAP_ENTRY_SIZE];
    uint16_t pos = 0;
    uint16_t start = (m_count < BRAVO_CAP_ENTRIES)
                     ? 0
                     : m_head;  // full: oldest is at head
    for (uint16_t k = 0; k < m_count; k++) {
        uint16_t idx = (uint16_t)((start + k) % BRAVO_CAP_ENTRIES);
        out[pos++] = (uint8_t)(m_cap[idx].cmd >> 8);
        out[pos++] = (uint8_t)(m_cap[idx].cmd & 0xFF);
        out[pos++] = (uint8_t)(m_cap[idx].len >> 8);
        out[pos++] = (uint8_t)(m_cap[idx].len & 0xFF);
        memcpy(&out[pos], m_cap[idx].data, BRAVO_CAP_DATA_LEN);
        pos += BRAVO_CAP_DATA_LEN;
    }
    return data_frame_make(cmd, STATUS_SUCCESS, pos, out);
}

data_frame_tx_t *bravo_cmd_clear_capture(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    m_head = 0;
    m_count = 0;
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}
