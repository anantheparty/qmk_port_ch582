/*
 * Obey65 2.4G Receiver Configuration
 * CH582F-based USB dongle for Obey65 keyboard
 */

#pragma once

/* USB Device descriptor parameter */
#define VENDOR_ID    0xCAFE
#define PRODUCT_ID   0x0B98  // Different from keyboard (0x0B97)
#define DEVICE_VER   0x0001
#define MANUFACTURER Obey
#define PRODUCT      Obey65 Dongle

/* Receiver has no matrix */
#define MATRIX_ROWS 0
#define MATRIX_COLS 0

/* Receiver GPIO */
// USB: PB10/PB11 (built-in)
// LED: PA8 (status indicator)
// Button: PA9 (pairing button)
#define DONGLE_LED_PIN      A8
#define DONGLE_BUTTON_PIN   A9

/* No RGB on receiver */
#undef RGB_MATRIX_ENABLE
#undef RGBLED_NUM

/* NKRO configuration */
#define NKRO_ENABLE
#define FORCE_NKRO

/* ESB Receiver mode */
// ESB_ENABLE = 2 means receiver/dongle mode
// This is set in rules.cmake

/* Disable features not needed for receiver */
#define NO_ACTION_LAYER
#define NO_ACTION_TAPPING
#define NO_ACTION_ONESHOT
