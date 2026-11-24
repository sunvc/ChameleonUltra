#ifndef MAX_BLE_GUARD_H
#define MAX_BLE_GUARD_H

#include <stdint.h>
#include <stdbool.h>

// MAX fork hook: called from the BLE GAP disconnect handler.
// Decides whether advertising may restart given the pending sleep path.
void max_ble_on_disconnected(void);

#endif
