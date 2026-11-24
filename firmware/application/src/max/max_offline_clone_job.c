// MAX fork: main-loop job orchestration for the offline Mifare Classic clone.
// The blocking clone engine lives in mf1_offline_clone.c; this file only
// handles request/tick state, reader-mode transitions and LED feedback.

#include <stdbool.h>
#include <stdint.h>

#include "nrf_gpio.h"

#include "app_status.h"
#include "bsp_delay.h"
#include "bsp_wdt.h"
#include "dataframe.h"
#include "hw_connect.h"
#include "mf1_offline_clone.h"
#include "rfid_main.h"
#include "rgb_marquee.h"
#include "tag_emulation.h"

#include "max_offline_clone_job.h"

#define NRF_LOG_MODULE_NAME max_clone
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
NRF_LOG_MODULE_REGISTER();

// BLE command IDs for this fork, kept out of the upstream data_cmd.h enum.
#define DATA_CMD_MAX_MF1_OFFLINE_CLONE        (2023)
#define DATA_CMD_MAX_MF1_OFFLINE_CLONE_STATUS (2024)

static void clone_led_indicate(uint8_t color, uint8_t times) {
    uint8_t slot = tag_emulation_get_slot();
    uint8_t led_slot = slot % RGB_LIST_NUM;
    uint8_t base_color = get_color_by_slot(slot);
    uint32_t *led_pins = hw_get_led_array();

    set_slot_light_color(color);
    for (uint8_t round = 0; round < 3; round++) {
        for (uint8_t i = 0; i < times; i++) {
            nrf_gpio_pin_set(led_pins[led_slot]);
            bsp_delay_ms(120);
            nrf_gpio_pin_clear(led_pins[led_slot]);
            bsp_delay_ms(120);
        }
        bsp_delay_ms(400);
    }
    set_slot_light_color(base_color);
    light_up_by_slot();
}

static void clone_run_once(void) {
    uint8_t slot_now = tag_emulation_get_slot();
    bool was_reader = get_device_mode() == DEVICE_MODE_READER;

    if (!was_reader) {
        reader_mode_enter();
        bsp_delay_ms(8);
    }

    rgb_marquee_stop();
    set_slot_light_color(RGB_BLUE);
    nrf_gpio_pin_set(hw_get_led_array()[slot_now % RGB_LIST_NUM]);

    (void)mf1_offline_clone_execute(slot_now);

    switch (mf1_offline_clone_job_state()) {
        case MF1_OFFLINE_CLONE_DONE_OK:
            clone_led_indicate(RGB_GREEN, 2);
            break;
        case MF1_OFFLINE_CLONE_DONE_NO_CARD:
            clone_led_indicate(RGB_RED, 1);
            break;
        case MF1_OFFLINE_CLONE_DONE_NOT_MF1:
            clone_led_indicate(RGB_RED, 2);
            break;
        case MF1_OFFLINE_CLONE_DONE_READ_FAIL:
        case MF1_OFFLINE_CLONE_DONE_APPLY_FAIL:
            clone_led_indicate(RGB_RED, 3);
            break;
        case MF1_OFFLINE_CLONE_DONE_KEYS_MISSING:
        default: {
            uint8_t missing = mf1_offline_clone_job_missing_sectors();
            if (missing == 0 || missing > 8) {
                missing = 8;
            }
            clone_led_indicate(RGB_RED, missing);
            break;
        }
    }

    if (!was_reader) {
        tag_mode_enter();
    }
}

void max_offline_clone_request(void) {
    mf1_offline_clone_job_request();
}

bool max_offline_clone_is_running(void) {
    return mf1_offline_clone_job_state() == MF1_OFFLINE_CLONE_RUNNING;
}

void max_offline_clone_tick(void) {
    if (!mf1_offline_clone_job_take_request()) {
        return;
    }
    if (mf1_offline_clone_job_state() == MF1_OFFLINE_CLONE_RUNNING) {
        return;
    }
    clone_run_once();
}

data_frame_tx_t *max_cmd_offline_clone(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    max_offline_clone_request();
    return data_frame_make(cmd, STATUS_SUCCESS, 0, NULL);
}

data_frame_tx_t *max_cmd_offline_clone_status(uint16_t cmd, uint16_t status, uint16_t length, uint8_t *data) {
    (void)status;
    (void)length;
    (void)data;
    uint8_t payload[2] = {
        mf1_offline_clone_job_state(),
        mf1_offline_clone_job_missing_sectors(),
    };
    return data_frame_make(cmd, STATUS_SUCCESS, sizeof(payload), payload);
}
