# Obey65 2.4G Receiver Build Configuration

# Basic features
set(MOUSE_ENABLE ON CACHE BOOL "KB" FORCE)
set(NKRO_ENABLE ON CACHE BOOL "KB" FORCE)
set(ENCODER_ENABLE OFF CACHE BOOL "KB" FORCE)
set(VIA_ENABLE OFF CACHE BOOL "KB" FORCE)
set(BOOTMAGIC_ENABLE OFF CACHE BOOL "KB" FORCE)
set(COMMAND_ENABLE OFF CACHE BOOL "KB" FORCE)

# Disable RGB
set(RGBLIGHT_ENABLE OFF CACHE BOOL "KB" FORCE)
set(RGB_MATRIX_ENABLE OFF CACHE BOOL "KB" FORCE)

# USB always enabled
set(USB_ENABLE ON CACHE BOOL "KB" FORCE)

# BLE disabled for receiver
set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)

# ESB in receiver/dongle mode (ESB_ENABLE = 2)
set(ESB_ENABLE 2 CACHE STRING "KB" FORCE)
set(ESB_ROLE "dongle" CACHE STRING "KB" FORCE)
# Receiver firmware uses local ESB implementation; wireless lib is optional.
set(WIRELESS_LIB_OPTIONAL ON CACHE BOOL "KB" FORCE)

# EEPROM for pairing info storage
set(EEPROM_ENABLE ON CACHE BOOL "KB" FORCE)
set(EEPROM_DRIVER "custom" CACHE STRING "KB" FORCE)

# Debug UART option
option(DEBUG_UART_ENABLE "Enable debug UART output" OFF)
if(DEBUG_UART_ENABLE)
    add_definitions(-DDEBUG_UART_ENABLE)
    list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/debug_uart.c")
    message(STATUS "Obey65 Receiver: Debug UART enabled (PA7, 115200 baud)")
endif()

# Add receiver-specific source files
list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/wireless_stubs.c")
list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/esb_receiver.c")
list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/obey65_receiver.c")

message(STATUS "Obey65 Receiver: Building 2.4G dongle firmware (ESB_ENABLE=2)")
