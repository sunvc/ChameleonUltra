// BRAVO fork: dalong-compatible field-wake auto polling.
// See docs/bravo-polling-spec.md for the behavior contract and which parts
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

#include "bravo_polling.h"

#define NRF_LOG_MODULE_NAME bravo_poll
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

// Private FDS record (the upstream id space in utils/fds_ids.h ends at 0x12xx).
#define BRAVO_POLLING_FDS_FILE_ID    0x1300
#define BRAVO_POLLING_FDS_RECORD_KEY 0x0001

#define BRAVO_POLLING_CFG_VERSION    3
#define BRAVO_POLLING_CFG_V2_SIZE    20  // 1+1+8+8+2, 8-slot layout
#define BRAVO_POLLING_DEFAULT_DELAY  0
#define BRAVO_POLLING_NO_SECTOR      0xFF

// BLE command IDs, companion-app compatible with the previous Max tree.
#define DATA_CMD_GET_DEVICE_AUTO_POLLING       (6038)
#define DATA_CMD_SET_DEVICE_AUTO_POLLING       (6039)
#define DATA_CMD_GET_DEVICE_AUTO_POLLING_DELAY (6040)
#define DATA_CMD_SET_DEVICE_AUTO_POLLING_DELAY (6041)

typedef struct {
    uint8_t  version;
    uint8_t  enabled;                         // global auto polling switch
    uint8_t  slot_enable[TAG_MAX_SLOT_NUM];   // per-slot poll switch
    uint8_t  last_auth_sector[TAG_MAX_SLOT_NUM]; // smart-poll sector memory
    uint16_t delay_ms;                        // pre-switch delay at field wake
} PACKED bravo_polling_config_t;

// Migration depends on these exact layouts
STATIC_ASSERT(sizeof(bravo_polling_config_t) == 68);
STATIC_ASSERT(BRAVO_POLLING_CFG_V2_SIZE == 20);

static bravo_polling_config_t m_cfg;
static bool m_runtime_disabled;   // latched by a button press, RAM only
static bool m_original_saved;
static uint8_t m_original_slot;
static uint8_t m_pending_block;   // block from the auth attempt in flight

static void cfg_set_defaults(void) {
    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.version = BRAVO_POLLING_CFG_VERSION;
    m_cfg.enabled = 0;
    for (uint8_t i = 0; i < TAG_MAX_SLOT_NUM; i++) {
        m_cfg.slot_enable[i] = 1;
        m_cfg.last_auth_sector[i] = BRAVO_POLLING_NO_SECTOR;
    }
    m_cfg.delay_ms = BRAVO_POLLING_DEFAULT_DELAY;
}

static void cfg_save(void) {
    if (!fds_write_sync(BRAVO_POLLING_FDS_FILE_ID, BRAVO_POLLING_FDS_RECORD_KEY,
                        sizeof(m_cfg), &m_cfg)) {
        NRF_LOG_WARNING("Save auto polling config list failed.");
    }
}

void bravo_polling_init(void) {
    cfg_set_defaults();
    m_runtime_disabled = false;
    m_original_saved = false;
    m_pending_block = 0;

    // Length-aware read: v2 records (20 bytes, 8 slots) migrate in place;
    // the remaining slot entries keep their defaults from cfg_set_defaults().
    bravo_polling_config_t stored;
    uint16_t len = sizeof(stored);
    if (fds_read_sync(BRAVO_POLLING_FDS_FILE_ID, BRAVO_POLLING_FDS_RECORD_KEY,
                      &len, (uint8_t *)&stored) &&
            stored.version == BRAVO_POLLING_CFG_VERSION &&
            len == sizeof(stored)) {
        m_cfg = stored;
    } else if (len == BRAVO_POLLING_CFG_V2_SIZE) {
        uint8_t *old = (uint8_t *)&stored;
        m_cfg.enabled = old[1];
        memcpy(m_cfg.slot_enable, &old[2], 8);
        memcpy(m_cfg.last_auth_sector, &old[2 + 8], 8);
        m_cfg.delay_ms = (uint16_t)((old[18] << 8) | old[19]);
        NRF_LOG_INFO("Auto polling config migrated v2 -> v3");
    }
}

// Smart-poll filter: any slot with a remembered non-zero auth sector turns
// on sector filtering for the next wake. Returns the filter sector, or
// BRAVO_POLLING_NO_SECTOR when plain round-robin applies.
static uint8_t smart_filter_sector(void) {
    uint8_t sector = BRAVO_POLLING_NO_SECTOR;
    for (uint8_t i = 0; i < TAG_MAX_SLOT_NUM; i++) {
        if (m_cfg.last_auth_sector[i] != BRAVO_POLLING_NO_SECTOR &&
                m_cfg.last_auth_sector[i] != 0) {
            if (sector == BRAVO_POLLING_NO_SECTOR) {
                sector = m_cfg.last_auth_sector[i];
            } else if (sector != m_cfg.last_auth_sector[i]) {
                // Conflicting sector memory: filtering would be ambiguous.
                return BRAVO_POLLING_NO_SECTOR;
            }
        }
    }
    return sector;
}

// Find the next pollable slot for the given sense type, round-robin after
// the active slot. When sector filtering is active, slots remembering the
// reader's last auth sector take priority. Returns TAG_MAX_SLOT_NUM when no
// candidate exists.
static uint8_t find_next_slot(tag_sense_type_t sense) {
    uint8_t active = tag_emulation_get_slot();
    uint8_t filter = smart_filter_sector();
    bool use_filter = filter != BRAVO_POLLING_NO_SECTOR;

    if (use_filter) {
        NRF_LOG_INFO("Smart poll enabled, filtering by sector %d", filter);
        for (uint8_t step = 1; step <= TAG_MAX_SLOT_NUM; step++) {
            uint8_t candidate = (uint8_t)((active + step) % TAG_MAX_SLOT_NUM);
            if (m_cfg.slot_enable[candidate] && is_slot_enabled(candidate, sense) &&
                    m_cfg.last_auth_sector[candidate] == filter) {
                return candidate;
            }
        }
        NRF_LOG_INFO("Smart poll: no valid next slot found");
        // Filter miss falls through to plain round-robin.
    }

    for (uint8_t step = 1; step <= TAG_MAX_SLOT_NUM; step++) {
        uint8_t candidate = (uint8_t)((active + step) % TAG_MAX_SLOT_NUM);
        if (m_cfg.slot_enable[candidate] && is_slot_enabled(candidate, sense)) {
            return candidate;
        }
    }
    return TAG_MAX_SLOT_NUM;
}

void bravo_polling_note_auth_block(uint8_t block) {
    // MF1 blocks map four per sector; trailer blocks are inside their sector.
    m_pending_block = block;
}

void bravo_polling_note_auth_result(bool success) {
    if (!success) {
        return;
    }
    uint8_t slot = tag_emulation_get_slot();
    uint8_t sector = m_pending_block / 4;
    if (m_cfg.last_auth_sector[slot] != sector) {
        m_cfg.last_auth_sector[slot] = sector;
        cfg_save();
    }
    if (sector != 0) {
        NRF_LOG_INFO("Non-zero sector auth success, adaptive polling continues");
    }
}

void bravo_polling_on_field_wake(bool hf) {
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

void bravo_polling_on_button(void) {
    if (m_cfg.enabled && !m_runtime_disabled) {
        m_runtime_disabled = true;
        NRF_LOG_INFO("Auto polling disabled by button press");
    }
}

void bravo_polling_on_sleep(void) {
    if (m_original_saved) {
        NRF_LOG_INFO("Auto polling: restoring original slot %d", m_original_slot);
        tag_emulation_change_slot(m_original_slot, false);
        m_original_saved = false;
    }
}

data_frame_tx_t *bravo_cmd_get_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    uint8_t payload = m_cfg.enabled ? 1 : 0;
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), &payload);
}

data_frame_tx_t *bravo_cmd_set_auto_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
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

data_frame_tx_t *bravo_cmd_get_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    uint8_t payload[2];
    payload[0] = (uint8_t)(m_cfg.delay_ms >> 8);
    payload[1] = (uint8_t)(m_cfg.delay_ms & 0xff);
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), payload);
}

data_frame_tx_t *bravo_cmd_set_auto_polling_delay(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length != 2) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    m_cfg.delay_ms = U16NTOHS(*(uint16_t *)data);
    cfg_save();
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}
