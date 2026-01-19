# Phase 2 阻塞问题: BLE 启动代码缺失

## 问题概述

当启用 `BLE_ENABLE=ON` 时，固件编译失败，链接器报告找不到 `_start` 符号。

## 错误信息

```
/Users/kamico/.local/opt/RISC-V Embedded GCC12/bin/../lib/gcc/riscv-wch-elf/12.2.0/../../../../riscv-wch-elf/bin/ld: warning: cannot find entry symbol _start; defaulting to 0000000000013000
Memory region         Used Size  Region Size  %age Used
       FLASH:          0 GB       372 KB      0.00%
         RAM:         512 B        32 KB      1.56%
```

## 根本原因

`sdk/CMakeLists.txt` 第 56-65 行：

```cmake
if(BUILD_WIRELESS_LIB OR(NOT BLE_ENABLE AND NOT ESB_ENABLE))
    list(APPEND ch582_sdk_temp
        "${CMAKE_CURRENT_LIST_DIR}/HAL/MCU.c"
        "${CMAKE_CURRENT_LIST_DIR}/HAL/RTC.c"
    )
    target_sources(CH582_APP INTERFACE
        ${ch582_sdk_temp}
        "${CMAKE_CURRENT_LIST_DIR}/Startup/startup_CH583.S"
    )
endif()
```

**问题**: 当 `BLE_ENABLE=ON` 时，如果 `BUILD_WIRELESS_LIB` 未设置：
- `startup_CH583.S` 不会被包含到 APP 目标中
- `_start` 符号不存在
- 链接器无法找到程序入口点

## 项目架构分析

项目设计为两种模式：

1. **无 BLE 模式** (`BLE_ENABLE=OFF`)
   - 使用标准启动代码 `startup_CH583.S`
   - 主程序入口点 `main()`
   - ✅ 工作正常

2. **BLE 模式** (`BLE_ENABLE=ON`)
   - 期望使用预编译的无线库
   - 无线库提供启动代码和 TMOS 调度器
   - ❌ 需要 `BUILD_WIRELESS_LIB` 或预编译库

## 可能的解决方案

### 方案 1: 修改 CMake 逻辑 (推荐)

修改 `sdk/CMakeLists.txt`，即使启用 BLE 也包含启动代码：

```cmake
# 始终包含启动代码
list(APPEND ch582_sdk_temp
    "${CMAKE_CURRENT_LIST_DIR}/HAL/MCU.c"
    "${CMAKE_CURRENT_LIST_DIR}/HAL/RTC.c"
)
target_sources(CH582_APP INTERFACE
    ${ch582_sdk_temp}
    "${CMAKE_CURRENT_LIST_DIR}/Startup/startup_CH583.S"
)
```

**风险**: 可能与 BLE 库的启动代码冲突

### 方案 2: 创建 BLE 兼容的启动代码

创建专门用于 BLE 模式的启动代码，正确初始化 TMOS 和 BLE 栈。

**需要**:
- 理解 WCH BLE 库的初始化要求
- 参考官方 BLE 示例代码

### 方案 3: 使用 BUILD_WIRELESS_LIB

如果有无线库源代码，启用 `BUILD_WIRELESS_LIB` 从源码编译。

**命令**:
```bash
cmake -DBUILD_WIRELESS_LIB=ON ...
```

**需要**: 无线库源代码（可能未公开）

### 方案 4: 参考现有 BLE 键盘实现

查看项目中其他支持 BLE 的键盘配置：
- `qmk_porting/keyboards/EC61/keymaps/bluetooth_le/`
- `qmk_porting/keyboards/mk67lite/keymaps/bluetooth_le/`
- `qmk_porting/keyboards/imk64/keymaps/bluetooth_le/`

检查它们是否有特殊配置或补丁。

## 现有 BLE 代码状态

`qmk_porting/keyboards/obey65/wireless/` 目录已包含：

- `src/protocol_ble.c` - BLE 协议实现
- `src/hid_dev.c` - HID 设备服务
- `src/protocol_esb.c` - ESB 协议实现
- `include/protocol_ble.h` - 头文件
- `include/protocol_esb.h` - 头文件

这些代码可以编译，但链接阶段失败。

## 下一步建议

1. **调研官方示例**: 查看 WCH 官方 CH582 BLE 示例项目的启动流程
2. **分析 BLE 库**: 检查 `LIBCH58xBLE.a` 是否导出 `_start` 或其他入口点
3. **咨询原始项目**: 联系 qmk_port_ch582 项目维护者了解 BLE 编译方式
4. **临时方案**: 继续使用 USB 模式，等待解决 BLE 问题

## 临时状态

- BLE_ENABLE=OFF (暂时禁用)
- Phase 1 功能正常工作
- 等待 BLE 问题解决后继续 Phase 2

---

*错误报告日期: 2026-01-19*
