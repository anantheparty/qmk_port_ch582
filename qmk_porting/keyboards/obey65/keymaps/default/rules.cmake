# Phase 2: BLE 功能 - 阻塞于无线库问题
# 问题: 当 BLE_ENABLE=ON 时，startup_CH583.S 不会被包含
# 原因: sdk/CMakeLists.txt:56 条件逻辑需要 BUILD_WIRELESS_LIB
# TODO: 需要解决无线库编译问题才能继续
set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)
set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)

# Phase 1: 启用调试输出 (UART2/PB23, 115200 baud)
set(DEBUG_UART_ENABLE ON CACHE BOOL "KB" FORCE)

message(STATUS "Obey65 default keymap: BLE=${BLE_ENABLE}, ESB=${ESB_ENABLE}, DEBUG_UART=${DEBUG_UART_ENABLE}")