// BRAVO fork: dalong/Nolet app 6000-range command compatibility.
//
// Sources: Nolet companion app bundle (chameleonCommands.js,
// chameleonUltraAdapter.js). The app speaks the same Proxgrind frame
// transport; the command IDs and payload formats here match that bundle.
// Paywall commands store data without enforcing any restriction, and the
// trial counter reports unlimited.

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "app_status.h"
#include "app_cmd.h"
#include "ble.h"
#include "ble_gap.h"
#include "dataframe.h"
#include "fds_util.h"
#include "settings.h"
#include "tag_emulation.h"

#include "bravo_compat.h"
#include "bravo_polling.h"

#define NRF_LOG_MODULE_NAME bravo_compat
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

// Private FDS record (upstream fds_ids.h ends in the 0x12xx/0x13xx range)
#define BRAVO_COMPAT_FDS_FILE_ID    0x1400
#define BRAVO_COMPAT_FDS_RECORD_KEY 0x0001

#define BRAVO_COMPAT_CFG_VERSION   1
#define BRAVO_KEY_LEN              48
#define BRAVO_BLE_NAME_LEN         31  // BLE GAP max adv-safe name length

// Dalong/Nolet command IDs
#define CMD_GET_LICENSE_KEY            6042
#define CMD_SET_LICENSE_KEY            6043
#define CMD_SET_SECRET_KEY             6044
#define CMD_GET_TRIAL_LEFT             6045
#define CMD_GET_IC_POLLING             6046
#define CMD_SET_IC_POLLING             6047
#define CMD_GET_ID_POLLING             6048
#define CMD_SET_ID_POLLING             6049
#define CMD_GET_CUSTOM_GIT             6050
#define CMD_GET_SLEEP_TIME             6051
#define CMD_SET_SLEEP_TIME             6052
#define CMD_SET_FENCE_KEY              6053
#define CMD_GET_DISABLE_SLEEP          6060
#define CMD_SET_DISABLE_SLEEP          6061
#define CMD_SET_SLOT_POLL_LIST         6001  // collides with upstream APDU_SEND
#define CMD_GET_SLOT_POLL_LIST         1763  // 0x6e3
#define CMD_GET_BLE_NAME               6076
#define CMD_SET_BLE_NAME               6077
#define CMD_SET_FENCE_ACTIVE_SLOT      6068
#define CMD_CHECK_FENCE_STATUS         6069

typedef struct {
    uint8_t version;
    uint8_t sleep_permanent;
    uint8_t license[BRAVO_KEY_LEN];
    uint8_t secret[BRAVO_KEY_LEN];
    uint8_t fence_key[BRAVO_KEY_LEN];
    uint8_t ble_name[BRAVO_BLE_NAME_LEN + 1];
    // FDS alignment padding, reserved for future use
    uint8_t reserved[2];
} PACKED bravo_compat_config_t;

STATIC_ASSERT(sizeof(bravo_compat_config_t) % 4 == 0);
STATIC_ASSERT(sizeof(bravo_compat_config_t) == 180);

static bravo_compat_config_t m_cfg;

static void cfg_defaults(void) {
    memset(&m_cfg, 0, sizeof(m_cfg));
    m_cfg.version = BRAVO_COMPAT_CFG_VERSION;
}

static void cfg_save(void) {
    if (!fds_write_sync(BRAVO_COMPAT_FDS_FILE_ID, BRAVO_COMPAT_FDS_RECORD_KEY,
                        sizeof(m_cfg), &m_cfg)) {
        NRF_LOG_WARNING("BRAVO compat config save failed.");
    }
}

static void cfg_load(void) {
    bravo_compat_config_t stored;
    uint16_t len = sizeof(stored);
    if (fds_read_sync(BRAVO_COMPAT_FDS_FILE_ID, BRAVO_COMPAT_FDS_RECORD_KEY,
                      &len, (uint8_t *)&stored) &&
            len == sizeof(stored) &&
            stored.version == BRAVO_COMPAT_CFG_VERSION) {
        m_cfg = stored;
    }
}

static void apply_ble_name(void) {
    if (m_cfg.ble_name[0] == 0) {
        return;
    }
    ble_gap_conn_sec_mode_t sec_mode;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);
    uint16_t name_len = (uint16_t)strlen((const char *)m_cfg.ble_name);
    if (name_len > 0) {
        (void)sd_ble_gap_device_name_set(&sec_mode, m_cfg.ble_name, name_len);
    }
}

void bravo_compat_init(void) {
    cfg_defaults();
    cfg_load();
    apply_ble_name();  // BLE stack is already initialized at this point
}

bool bravo_compat_sleep_blocked(void) {
    return m_cfg.sleep_permanent != 0;
}

static data_frame_tx_t *ok_empty(uint16_t cmd) {
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}

static void store_key(uint8_t *dst, const uint8_t *data, uint16_t length) {
    memset(dst, 0, BRAVO_KEY_LEN);
    if (length > 0) {
        uint16_t n = length > BRAVO_KEY_LEN ? BRAVO_KEY_LEN : length;
        memcpy(dst, data, n);
    }
}

static data_frame_tx_t *get_stored_string(uint16_t cmd, const uint8_t *buf) {
    uint16_t len = 0;
    while (len < BRAVO_KEY_LEN && buf[len] != 0) {
        len++;
    }
    return data_frame_make(cmd, STATUS_SUCCESS, len, (uint8_t *)buf);
}

data_frame_tx_t *bravo_cmd_get_license_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return get_stored_string(cmd, m_cfg.license);
}

data_frame_tx_t *bravo_cmd_set_license_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    store_key(m_cfg.license, data, length);
    cfg_save();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_set_secret_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    store_key(m_cfg.secret, data, length);
    cfg_save();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_get_trial(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    // No paywall: report 0 used, 65535 remaining (big-endian per app parser)
    uint8_t payload[4] = {0x00, 0x00, 0xFF, 0xFF};
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), payload);
}

static data_frame_tx_t *get_gate(uint16_t cmd, bool hf) {
    uint8_t v = bravo_polling_get_sense_gate(hf) ? 1 : 0;
    return data_frame_make(cmd, STATUS_SUCCESS, 1, &v);
}

static data_frame_tx_t *set_gate(uint16_t cmd, uint16_t length, uint8_t *data, bool hf) {
    if (length != 1) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    bravo_polling_set_sense_gate(hf, data[0] != 0);
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_get_ic_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return get_gate(cmd, true);
}

data_frame_tx_t *bravo_cmd_set_ic_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    return set_gate(cmd, length, data, true);
}

data_frame_tx_t *bravo_cmd_get_id_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return get_gate(cmd, false);
}

data_frame_tx_t *bravo_cmd_set_id_polling(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    return set_gate(cmd, length, data, false);
}

data_frame_tx_t *bravo_cmd_get_custom_git(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return data_frame_make(cmd, STATUS_SUCCESS, strlen(GIT_VERSION), (uint8_t *)GIT_VERSION);
}

data_frame_tx_t *bravo_cmd_get_sleep_time(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    // settings stores seconds; getter returns milliseconds, convert back
    uint8_t seconds = (uint8_t)(settings_get_sleep_timeout() / 1000U);
    return data_frame_make(cmd, STATUS_SUCCESS, 1, &seconds);
}

data_frame_tx_t *bravo_cmd_set_sleep_time(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length != 1) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    // App clamps 1..20; upstream settings range is 5..60
    uint8_t seconds = data[0] < 5 ? 5 : (data[0] > 60 ? 60 : data[0]);
    settings_set_sleep_timeout(seconds);
    settings_save_config();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_set_fence_key(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    store_key(m_cfg.fence_key, data, length);
    cfg_save();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_get_disable_sleep(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    uint8_t v = m_cfg.sleep_permanent;
    return data_frame_make(cmd, STATUS_SUCCESS, 1, &v);
}

data_frame_tx_t *bravo_cmd_set_disable_sleep(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length != 1) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    m_cfg.sleep_permanent = data[0] ? 1 : 0;
    cfg_save();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_get_ble_name(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return get_stored_string(cmd, m_cfg.ble_name);
}

data_frame_tx_t *bravo_cmd_set_ble_name(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length == 0 || length > BRAVO_BLE_NAME_LEN) {
        return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
    }
    memset(m_cfg.ble_name, 0, sizeof(m_cfg.ble_name));
    memcpy(m_cfg.ble_name, data, length);
    apply_ble_name();
    cfg_save();
    return ok_empty(cmd);
}

data_frame_tx_t *bravo_cmd_get_slot_poll_list(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    uint8_t list[TAG_MAX_SLOT_NUM];
    bravo_polling_get_slot_list(list);
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(list), list);
}

// Re-wrap an inner command response under an outer fence command id.
// Inner frame fields are big-endian per the Proxgrind frame spec.
static data_frame_tx_t *rewrap(uint16_t outer_cmd, data_frame_tx_t *sub) {
    if (sub == NULL) {
        return data_frame_make(outer_cmd, STATUS_CMD_ERR, 0, NULL);
    }
    uint16_t inner_status = (uint16_t)(((uint16_t)sub->buffer[4] << 8) | sub->buffer[5]);
    uint16_t payload_len = (uint16_t)(((uint16_t)sub->buffer[6] << 8) | sub->buffer[7]);
    return data_frame_make(outer_cmd, inner_status, payload_len, &sub->buffer[9]);
}

data_frame_tx_t *bravo_cmd_set_fence_active_slot(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    // Companion app sends SET_ACTIVE_SLOT_BY_FENCE as a 1-byte slot index;
    // activate that slot (fence-triggered switch).
    if (length == 1) {
        uint8_t slot = data[0];
        if (slot >= TAG_MAX_SLOT_NUM) {
            return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
        }
        tag_emulation_change_slot(slot, true);
        return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
    }
    // Legacy forwarder form: 3-byte [slot, subcmd BE16].
    if (length == 3 && data[0] < TAG_MAX_SLOT_NUM) {
        uint16_t subcmd = (uint16_t)(((uint16_t)data[1] << 8) | data[2]);
        uint8_t original = tag_emulation_get_slot();
        tag_emulation_set_slot(data[0]);
        data_frame_tx_t *sub = app_cmd_execute(subcmd, 0, 0, NULL);
        tag_emulation_set_slot(original);
        return rewrap(cmd, sub);
    }
    return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
}

data_frame_tx_t *bravo_cmd_ack(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status; (void)length; (void)data;
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}

// 6072: fence card type write, 5 bytes [slot, sense, enable, tag_type BE16].
data_frame_tx_t *bravo_cmd_set_fence_card(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    if (length == 5) {
        uint8_t slot = data[0];
        uint16_t type = (uint16_t)(((uint16_t)data[3] << 8) | data[4]);
        if (slot < TAG_MAX_SLOT_NUM && is_tag_specific_type_valid((tag_specific_type_t)type)) {
            tag_emulation_change_type(slot, (tag_specific_type_t)type);
            if (data[2] != 0) {
                tag_emulation_slot_set_enable(slot, get_sense_type_from_tag_type((tag_specific_type_t)type), true);
            }
            return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
        }
    }
    return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
}

// 6064: fence anti-collision write. Payload is identical to upstream
// cmd 4001 (uidlen|uid|atqa|sak|atslen|ats), so reuse that handler.
data_frame_tx_t *bravo_cmd_fence_anticoll(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    return rewrap(cmd, app_cmd_execute(4001, status, length, data));
}

// 6067: fence MF1 block write. Payload matches upstream 4000
// ([block_index][16-byte blocks...]), so reuse that handler.
data_frame_tx_t *bravo_cmd_fence_block_write(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    return rewrap(cmd, app_cmd_execute(4000, status, length, data));
}

data_frame_tx_t *bravo_cmd_check_fence_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    // The companion app probes fence support with a zero-length 6069 and
    // classifies the response by its status/payload. Answer "supported".
    if (length == 0) {
        uint8_t ready = 1;
        return data_frame_make(cmd, STATUS_SUCCESS, 1, &ready);
    }
    // With a 2-byte payload [subcmd BE16], run that command on the current
    // slot and return its result (dalong forwarder semantics).
    if (length == 2) {
        uint16_t subcmd = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
        return rewrap(cmd, app_cmd_execute(subcmd, 0, 0, NULL));
    }
    return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
}

data_frame_tx_t *bravo_compat_intercept(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    // 6001 collides with upstream HF14A_4_APDU_SEND. The dalong/Nolet app
    // always uses it as the 32-slot poll list, so route it here before the
    // upstream table sees it (APDU_SEND becomes unreachable).
    if (cmd == CMD_SET_SLOT_POLL_LIST) {
        if (!bravo_polling_set_slot_list(data, length)) {
            return data_frame_make(cmd, STATUS_PAR_ERR, 0, NULL);
        }
        return ok_empty(cmd);
    }
    (void)status;
    return NULL;
}
