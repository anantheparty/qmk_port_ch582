# Phase 0: 暂时禁用无线功能，确保有线模式稳定
# TODO: Phase 2 时重新启用 BLE
# TODO: Phase 3 时重新启用 ESB
set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)
set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)

# Phase 1: 启用调试输出 (UART2/PB23, 115200 baud)
set(DEBUG_UART_ENABLE ON CACHE BOOL "KB" FORCE)

message(STATUS "Obey65 default keymap: BLE=${BLE_ENABLE}, ESB=${ESB_ENABLE}, DEBUG_UART=${DEBUG_UART_ENABLE}")