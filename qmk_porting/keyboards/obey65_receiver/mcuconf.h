/*
 * Obey65 2.4G Receiver - MCU Configuration
 */

#pragma once

#define DEBUG_BAUDRATE       115200
#define DCDC_ENABLE          1
#define FREQ_SYS             40000000
#define LSE_ENABLE           1
#define HSE_LOAD_CAPACITANCE 20  // in pF unit

// No BLE slots needed - receiver uses ESB only
#define BLE_SLOT_NUM         0
