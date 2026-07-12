set(OBEY65_PROFILE "wired" CACHE STRING "Obey65 firmware profile")
set_property(CACHE OBEY65_PROFILE PROPERTY STRINGS wired ble-smoke ble-hid ble-qmk ble-dev)

# Keep the verified wired path and BLE experiments as separate artifacts.
if(OBEY65_PROFILE STREQUAL "wired")
    set(USB_ENABLE ON CACHE BOOL "KB" FORCE)
    set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
elseif(OBEY65_PROFILE STREQUAL "ble-smoke")
    # Advertising-only experiment: no USB, VIA, RGB matrix, or QMK task loop.
    set(USB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(BLE_ENABLE ON CACHE BOOL "KB" FORCE)
    set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(VIA_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(COMMAND_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(MOUSE_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(EEPROM_ENABLE ON CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_WS2812 OFF CACHE BOOL "KB" FORCE)
    set(RGB_RAW_ENABLE OFF CACHE BOOL "KB" FORCE)
    add_compile_definitions(OBEY65_BLE_SMOKE_TEST=1)
elseif(OBEY65_PROFILE STREQUAL "ble-hid")
    # Connectable HOGP service, without QMK matrix scanning or RGB.
    set(USB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(BLE_ENABLE ON CACHE BOOL "KB" FORCE)
    set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(VIA_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(COMMAND_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(MOUSE_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(EEPROM_ENABLE ON CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_WS2812 OFF CACHE BOOL "KB" FORCE)
    set(RGB_RAW_ENABLE OFF CACHE BOOL "KB" FORCE)
    add_compile_definitions(OBEY65_BLE_NO_QMK_TEST=1)
elseif(OBEY65_PROFILE STREQUAL "ble-qmk")
    # BLE HOGP + QMK matrix/report loop, without USB, VIA, or RGB.
    set(USB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(BLE_ENABLE ON CACHE BOOL "KB" FORCE)
    set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(VIA_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(COMMAND_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(MOUSE_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(EEPROM_ENABLE ON CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_ENABLE OFF CACHE BOOL "KB" FORCE)
    set(RGB_MATRIX_WS2812 OFF CACHE BOOL "KB" FORCE)
    set(RGB_RAW_ENABLE OFF CACHE BOOL "KB" FORCE)
elseif(OBEY65_PROFILE STREQUAL "ble-dev")
    # Integration build. Hardware behavior is not yet verified.
    set(USB_ENABLE ON CACHE BOOL "KB" FORCE)
    set(BLE_ENABLE ON CACHE BOOL "KB" FORCE)
    set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
else()
    message(FATAL_ERROR "Unknown OBEY65_PROFILE='${OBEY65_PROFILE}'")
endif()

if(BLE_ENABLE)
    set(KEYBOARD_WIRELESS_SOURCES
        "${KEYBOARD_ROOT}/wireless/src/ble_support.c"
        "${KEYBOARD_ROOT}/wireless/src/protocol_ble.c"
    )
    if(NOT OBEY65_PROFILE STREQUAL "ble-smoke")
        list(APPEND KEYBOARD_WIRELESS_SOURCES
            "${KEYBOARD_ROOT}/wireless/src/hid_dev.c"
        )
    endif()
elseif(ESB_ENABLE)
    set(KEYBOARD_WIRELESS_SOURCES
        "${KEYBOARD_ROOT}/wireless/src/ble_support.c"
        "${KEYBOARD_ROOT}/wireless/src/protocol_esb.c"
    )
endif()

option(OBEY65_USE_LSE "Use the board's external 32.768 kHz crystal" OFF)
if(OBEY65_USE_LSE)
    if(RGB_MATRIX_ENABLE OR RGB_MATRIX_WS2812 OR RGB_RAW_ENABLE)
        message(FATAL_ERROR "OBEY65_USE_LSE is only valid for BLE profiles without WS2812 PWM")
    endif()
    add_compile_definitions(CH58X_ALLOW_EXTERNAL_LSE=1 LSE_ENABLE=1)
    set(FIRMWARE_VARIANT "${OBEY65_PROFILE}-lse" CACHE STRING "Firmware filename suffix" FORCE)
else()
    set(FIRMWARE_VARIANT "${OBEY65_PROFILE}" CACHE STRING "Firmware filename suffix" FORCE)
endif()

add_compile_definitions(OBEY65_BUILD_PROFILE="${OBEY65_PROFILE}")

# The board has no usable UART header. Keep UART logging opt-in only.
set(DEBUG_UART_ENABLE OFF CACHE BOOL "KB" FORCE)

message(STATUS
    "Obey65 profile=${OBEY65_PROFILE}, USB=${USB_ENABLE}, BLE=${BLE_ENABLE}, "
    "ESB=${ESB_ENABLE}, VIA=${VIA_ENABLE}, external-LSE=${OBEY65_USE_LSE}")
