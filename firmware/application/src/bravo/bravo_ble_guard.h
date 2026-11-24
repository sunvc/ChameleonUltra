#ifndef BRAVO_BLE_GUARD_H
#define BRAVO_BLE_GUARD_H

#include <stdint.h>
#include <stdbool.h>

// BRAVO fork hook: called from the BLE GAP disconnect handler.
// Decides whether advertising may restart given the pending sleep path.
void bravo_ble_on_disconnected(void);

#endif
