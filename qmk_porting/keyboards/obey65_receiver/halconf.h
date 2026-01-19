/*
 * Obey65 2.4G Receiver - HAL Configuration
 */

#pragma once

// No UART remapping for receiver (default pins)
// UART2: PA6/PA7 used for debug

// No SPI needed for receiver

// RF TX Power (same as keyboard for matching)
#define BLE_TX_POWER LL_TX_POWEER_4_DBM
