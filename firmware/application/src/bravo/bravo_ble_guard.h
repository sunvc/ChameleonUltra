#ifndef BRAVO_BLE_GUARD_H
#define BRAVO_BLE_GUARD_H

#include <stdint.h>
#include <stdbool.h>

// BRAVO fork hook: called from the BLE GAP disconnect handler.
// Restarts advertising when the device will stay awake. Returns true when
// the caller must skip the disconnect sleep timer (permanent-awake mode).
bool bravo_ble_on_disconnected(void);

#endif
