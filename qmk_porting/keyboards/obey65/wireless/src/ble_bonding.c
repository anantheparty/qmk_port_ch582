/**
 * BLE Bonding Management
 *
 * Manages bonding slots for multi-device pairing.
 * Uses the CH582 GAPBondMgr for actual bonding operations.
 */

#include "ble_bonding.h"
#include "CH58xBLE_LIB.H"

#ifdef DEBUG_UART_ENABLE
#include "debug_uart.h"
// Use DEBUG_PRINTF for formatted output, DEBUG_PRINT for simple strings
#endif

// ============================================================================
// Bonding Slot State
// ============================================================================

static struct {
    uint8_t active_slot;                      // Currently active slot
    bond_slot_info_t slots[BLE_MAX_BONDS];    // Slot information
    bool initialized;
} bonding_state = {
    .active_slot = 0,
    .initialized = false
};

// ============================================================================
// Internal Helpers
// ============================================================================

static void update_slot_info(void) {
    uint8_t bond_count = 0;
    GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT, &bond_count);

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("BLE Bond: count=%d\n", bond_count);
#endif

    // Mark all slots as empty initially
    for (int i = 0; i < BLE_MAX_BONDS; i++) {
        bonding_state.slots[i].state = BOND_SLOT_EMPTY;
    }

    // For now, we track bonds linearly by slot number
    // The actual bond data is managed by GAPBondMgr internally
    // We just track which slots have been used
    for (int i = 0; i < bond_count && i < BLE_MAX_BONDS; i++) {
        bonding_state.slots[i].state = BOND_SLOT_BONDED;
    }

    // Mark active slot
    if (bonding_state.active_slot < BLE_MAX_BONDS) {
        if (bonding_state.slots[bonding_state.active_slot].state == BOND_SLOT_BONDED) {
            bonding_state.slots[bonding_state.active_slot].state = BOND_SLOT_ACTIVE;
        }
    }
}

// ============================================================================
// Public API
// ============================================================================

void ble_bonding_init(void) {
    if (bonding_state.initialized) {
        return;
    }

    bonding_state.active_slot = 0;
    bonding_state.initialized = true;

    // Load current bond status
    update_slot_info();

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("BLE Bond: Initialized, active=%d\n", bonding_state.active_slot);
#endif
}

uint8_t ble_bonding_get_count(void) {
    uint8_t count = 0;
    GAPBondMgr_GetParameter(GAPBOND_BOND_COUNT, &count);
    return count;
}

bool ble_bonding_get_slot_info(uint8_t slot, bond_slot_info_t *info) {
    if (slot >= BLE_MAX_BONDS || info == NULL) {
        return false;
    }

    *info = bonding_state.slots[slot];
    return true;
}

bool ble_bonding_set_active_slot(uint8_t slot) {
    if (slot >= BLE_MAX_BONDS) {
        return false;
    }

    bonding_state.active_slot = slot;
    update_slot_info();

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("BLE Bond: Active slot=%d\n", slot);
#endif

    return true;
}

uint8_t ble_bonding_get_active_slot(void) {
    return bonding_state.active_slot;
}

bool ble_bonding_clear_slot(uint8_t slot) {
    if (slot >= BLE_MAX_BONDS) {
        return false;
    }

    // Note: GAPBondMgr_SetParameter with GAPBOND_ERASE_SINGLEBOND
    // requires device address. For now, we don't track addresses per slot.
    // A full implementation would need to store addresses.

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINTF("BLE Bond: Clear slot %d (not fully implemented)\n", slot);
#endif

    // Mark slot as empty in our tracking
    bonding_state.slots[slot].state = BOND_SLOT_EMPTY;

    return true;
}

void ble_bonding_clear_all(void) {
#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("BLE Bond: Clearing all bonds\n");
#endif

    // Erase all bonds
    GAPBondMgr_SetParameter(GAPBOND_ERASE_ALLBONDS, 0, NULL);

    // Reset our tracking
    for (int i = 0; i < BLE_MAX_BONDS; i++) {
        bonding_state.slots[i].state = BOND_SLOT_EMPTY;
        tmos_memset(bonding_state.slots[i].addr, 0, 6);
        bonding_state.slots[i].addr_type = 0;
    }

    update_slot_info();
}

void ble_bonding_save(void) {
    // Force save bonds to flash
    GAPBondMgr_SetParameter(GAPBOND_BOND_UPDATE, 0, NULL);

#ifdef DEBUG_UART_ENABLE
    DEBUG_PRINT("BLE Bond: Saved to flash\n");
#endif
}

void ble_bonding_on_pair_complete(uint8_t *addr, uint8_t addr_type) {
    uint8_t slot = bonding_state.active_slot;

    if (slot < BLE_MAX_BONDS && addr != NULL) {
        tmos_memcpy(bonding_state.slots[slot].addr, addr, 6);
        bonding_state.slots[slot].addr_type = addr_type;
        bonding_state.slots[slot].state = BOND_SLOT_ACTIVE;

#ifdef DEBUG_UART_ENABLE
        DEBUG_PRINTF("BLE Bond: Paired to slot %d, addr=%02X:%02X:%02X:%02X:%02X:%02X\n",
                    slot, addr[5], addr[4], addr[3], addr[2], addr[1], addr[0]);
#endif
    }

    update_slot_info();
}

void ble_bonding_on_bond_erased(uint8_t *addr, uint8_t addr_type) {
    // Find and clear the slot for this address
    for (int i = 0; i < BLE_MAX_BONDS; i++) {
        if (bonding_state.slots[i].state != BOND_SLOT_EMPTY &&
            bonding_state.slots[i].addr_type == addr_type &&
            tmos_memcmp(bonding_state.slots[i].addr, addr, 6) == 0) {

            bonding_state.slots[i].state = BOND_SLOT_EMPTY;
            tmos_memset(bonding_state.slots[i].addr, 0, 6);

#ifdef DEBUG_UART_ENABLE
            DEBUG_PRINTF("BLE Bond: Erased slot %d\n", i);
#endif
            break;
        }
    }

    update_slot_info();
}
