# Phase 0: 暂时禁用无线功能，确保有线模式稳定
# TODO: Phase 2 时重新启用 BLE
# TODO: Phase 3 时重新启用 ESB
set(BLE_ENABLE OFF CACHE BOOL "KB" FORCE)
set(ESB_ENABLE OFF CACHE BOOL "KB" FORCE)
message(STATUS "Obey65 default keymap (wired-only), BLE_ENABLE=${BLE_ENABLE}, ESB_ENABLE=${ESB_ENABLE}")