// MAX fork: dalong-compatible field-wake auto polling.
// See docs/max-polling-spec.md for the behavior contract and which parts
// are binary-evidenced versus inferred.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_status.h"
#include "bsp_delay.h"
#include "dataframe.h"
#include "fds_util.h"
#include "tag_emulation.h"
#include "utils.h"

#include "max_polling.h"

#define NRF_LOG_MODULE_NAME max_poll
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

// Private FDS record (the upstream id space in utils/fds_ids.h ends at 0x12xx).
#define MAX_POLLING_FDS_FILE_ID    0x1300
#define MAX_POLLING_FDS_RECORD_KEY 0x0001

#define MAX_POLLING_CFG_VERSION    1
#define MAX_POLLING_DEFAULT_DELAY  0

// BLE command IDs, companion-app compatible with the previous Max tree.
#define DATA_CMD_GET_DEVICE_AUTO_POLLING       (6038)
#define DATA_CMD_SET_DEVICE_AUTO_POLLING       (6039)
#define DATA_CMD_GET_DEVICE_AUTO_POLLING_DELAY (6040)
#define DATA_CMD_SET_DEVICE_AUTO_POLLING_DELAY (6041)

typedef struct {
    uint8_t  version;
    uint8_t  enabled;                         // global auto polling switch
    uint8_t  slot_enable[TAG_MAX_SLOT_NUM];   // per-slot poll switch
    uint16_t delay_ms;                        // pre-switch delay at field wake
} PACKED max_polling_config_t;

static max_polling_config_t m_cfg;
static bool m_runtime_disabled;   // latched by a button press, RAM only
static bool m_original_saved;
static uint8_t m_original_slot;

static void cfg_set_defaults(void) {
    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.version = MAX_POLLING_CFG_VERSION;
    m_cfg.enabled = 0;
    for (uint8_t i = 0; i < TAG_MAX_SLOT_NUM; i++) {
        m_cfg.slot_enable[i] = 1;
    }
    m_cfg.delay_ms = MAX_POLLING_DEFAULT_DELAY;
}

static void cfg_save(void) {
    if (!fds_write_sync(MAX_POLLING_FDS_FILE_ID, MAX_POLLING_FDS_RECORD_KEY,
                        sizeof(m_cfg), &m_cfg)) {
        NRF_LOG_WARNING("Save auto polling config list failed.");
    }
}

void max_polling_init(void) {
    cfg_set_defaults();
    m_runtime_disabled = false;
    m_original_saved = false;

    max_polling_config_t stored;
    uint16_t len = sizeof(stored);
    if (fds_read_sync(MAX_POLLING_FDS_FILE_ID, MAX_POLLING_FDS_RECORD_KEY,
                      &len, (uint8_t *)&stored) &&
            len == sizeof(stored) &&
            stored.version == MAX_POLLING_CFG_VERSION) {
        m_cfg = stored;
    }
}

// Find the next pollable slot for the given sense type, round-robin after
// the active slot. Returns TAG_MAX_SLOT_NUM when no candidate exists.
static uint8_t find_next_slot(tag_sense_type_t sense) {
    uint8_t active = tag_emulation_get_slot();
    for (uint8_t step = 1; step <= TAG_MAX_SLOT_NUM; step++) {
        uint8_t candidate = (uint8_t)((active + step) % TAG_MAX_SLOT_NUM);
        if (m_cfg.slot_enable[candidate] && is_slot_enabled(candidate, sense)) {
            return candidate;
        }
    }
    return TAG_MAX_SLOT_NUM;
}

void max_polling_on_field_wake(bool hf) {
    if (!m_cfg.enabled || m_runtime_disabled) {
        NRF_LOG_DEBUG("Smart poll: auto_polling not enabled in settings");
        return;
    }

    tag_sense_type_t sense = hf ? TAG_SENSE_HF : TAG_SENSE_LF;
    uint8_t next = find_next_slot(sense);
    if (next == TAG_MAX_SLOT_NUM) {
        NRF_LOG_INFO("Smart poll: no valid next slot found");
        if (hf) {
            NRF_LOG_INFO("Auto polling only LF slots");
        } else {
            NRF_LOG_INFO("Auto polling only HF slots");
        }
        return;
    }

    if (m_cfg.delay_ms > 0) {
        bsp_delay_ms(m_cfg.delay_ms);
    }

    if (!m_original_saved) {
        m_original_slot = tag_emulation_get_slot();
        m_original_saved = true;
    }

    tag_emulation_change_slot(next, true);
    NRF_LOG_INFO("Auto polling switched to slot %d", next);
}

void max_polling_on_button(void) {
    if (m_cfg.enabled && !m_runtime_disabled) {
        m_runtime_disabled = true;
        NRF_LOG_INFO("Auto polling disabled by button press");
    }
}

void max_polling_on_sleep(void) {
    if (m_original_saved) {
        NRF_LOG_INFO("Auto polling: restoring original slot %d", m_original_slot);
        tag_emulation_change_slot(m_original_slot, false);
        m_original_saved = false;
    }
}

data_frame_tx_t *max_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    uint8_t payload = m_cfg.enabled ? 1 : 0;
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), &payload);
}

data_frame_tx_t *max_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length != 1) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    m_cfg.enabled = data[0] ? 1 : 0;
    if (m_cfg.enabled) {
        m_runtime_disabled = false;
    }
    cfg_save();
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}

data_frame_tx_t *max_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    uint8_t payload[2];
    payload[0] = (uint8_t)(m_cfg.delay_ms >> 8);
    payload[1] = (uint8_t)(m_cfg.delay_ms & 0xff);
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), payload);
}

data_frame_tx_t *max_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length != 2) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    m_cfg.delay_ms = U16NTOHS(*(uint16_t *)data);
    cfg_save();
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}
