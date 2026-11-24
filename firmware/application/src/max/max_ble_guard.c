// MAX fork: BLE disconnect policy.
// On disconnect the upstream handler always starts a short sleep timer.
// Restarting advertising unconditionally lets a host reconnect while the
// System OFF path is already running, which derails shutdown. Re-advertise
// only when the device will stay awake anyway (USB power, active tag
// emulation or field generator); otherwise let it power off.

#include "max_ble_guard.h"
#include "ble_main.h"
#include "tag_emulation.h"
#include "nrfx_power.h"

// Defined as a global in app_main.c (upstream already exposes it).
extern volatile bool m_is_field_on;

void max_ble_on_disconnected(void) {
    if (nrfx_power_usbstatus_get() != NRFX_POWER_USB_STATE_DISCONNECTED ||
            g_is_tag_emulating || m_is_field_on) {
        advertising_start(false);
    }
}
