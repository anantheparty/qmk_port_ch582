#pragma once

#include <stdint.h>
#include <stdbool.h>

// Maximum number of bonded devices (slots)
#define BLE_MAX_BONDS 4

// Bonding slot states
typedef enum {
    BOND_SLOT_EMPTY = 0,
    BOND_SLOT_BONDED,
    BOND_SLOT_ACTIVE
} bond_slot_state_t;

// Bonded device information
typedef struct {
    uint8_t addr[6];           // Device address
    uint8_t addr_type;         // Address type (public/random)
    bond_slot_state_t state;   // Slot state
} bond_slot_info_t;

// Initialize bonding management
void ble_bonding_init(void);

// Get number of bonded devices
uint8_t ble_bonding_get_count(void);

// Get information about a bonding slot
bool ble_bonding_get_slot_info(uint8_t slot, bond_slot_info_t *info);

// Set the active bonding slot (for reconnection)
bool ble_bonding_set_active_slot(uint8_t slot);

// Get current active slot
uint8_t ble_bonding_get_active_slot(void);

// Clear a specific bonding slot
bool ble_bonding_clear_slot(uint8_t slot);

// Clear all bonding information
void ble_bonding_clear_all(void);

// Save current bonding to flash (called automatically, but can force)
void ble_bonding_save(void);

// Called when pairing completes - associates new bond with current slot
void ble_bonding_on_pair_complete(uint8_t *addr, uint8_t addr_type);

// Called when bond is erased by system
void ble_bonding_on_bond_erased(uint8_t *addr, uint8_t addr_type);
