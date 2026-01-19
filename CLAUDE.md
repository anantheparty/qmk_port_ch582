# QMK Port CH582 - 项目配置

## 项目概述
这是 QMK 键盘固件向 CH582 (WCH RISC-V BLE SoC) 平台的移植项目。

## 技术栈
- **MCU**: CH582 (RISC-V 32-bit + BLE 5.3)
- **固件框架**: QMK Firmware (子模块)
- **USB 栈**: CherryUSB
- **Bootloader**: MCUBoot
- **构建系统**: CMake + Ninja
- **工具链**: WCH RISC-V 工具链 或 xpack-riscv-none-elf-gcc

## 项目结构
```
qmk_port_ch582/
├── sdk/                    # CH582 SDK (HAL)
├── CherryUSB/              # USB 子模块
├── mcuboot/                # Bootloader 子模块
├── qmk_firmware/           # QMK 子模块
├── qmk_porting/            # 适配层 (核心开发区域)
│   ├── keyboards/          # 键盘定义
│   ├── platforms/ch58x/    # 平台驱动
│   ├── protocol/           # 协议实现
│   └── common_features.cmake
├── CherryUSB_porting/      # USB 适配
└── mcuboot_porting/        # Bootloader 适配
```

## 编码规范

### C 代码风格
- 使用 QMK 代码风格 (4 空格缩进)
- 头文件使用 `#pragma once`
- 避免 `config.h` 命名冲突：使用完整路径 `#include "users/xxx/xxx.h"`

### CMake 规范
- 新功能通过 `add_definitions(-DFEATURE_ENABLE)` 启用
- 驱动源文件添加到 `list(APPEND QMK_PORTING_SOURCES ...)`

### 键盘定义
每个键盘需要：
- `config.h` - 硬件配置 (矩阵大小、引脚定义)
- `halconf.h` - HAL 配置
- `mcuconf.h` - MCU 配置
- `keyboard.json` 或 `rules.mk` - 功能开关
- `keymap.c` - 键位映射

## 重要注意事项

### ⚠️ 已知问题
1. **config.h 冲突**: sdk/HAL/include 和 QMK 都有 config.h，使用 include 时用完整路径
2. **中断处理**: 使用公版工具链必须定义 `INT_SOFT` 宏
3. **蓝牙功能**: 无线库暂未公开，蓝牙相关代码可能无法编译

### 构建命令
```bash
# 初始化子模块
git -c submodule."qmk_porting/keyboards_private".update=none submodule update --recursive --init

# 构建固件 (替换 keyboard_name 和 keymap_name)
mkdir build && cd build
cmake -G Ninja -DKEYBOARD=keyboard_name -DKEYMAP=keymap_name ..
ninja
```

## 开发工作流

### 添加新键盘
1. 在 `qmk_porting/keyboards/` 创建键盘目录
2. 定义 config.h (MATRIX_ROWS, MATRIX_COLS, 引脚)
3. 创建 keymap 目录和 keymap.c
4. 更新 CMakeLists.txt 添加新键盘

### 修改前检查
- [ ] 确认修改不会破坏现有键盘编译
- [ ] 测试 USB 和蓝牙模式 (如适用)
- [ ] 检查 RAM/Flash 使用量

## 调试技巧
- 使用 WCH-Link 进行 SWD 调试
- 串口日志通过 UART1 输出
- Bootmagic Lite: 按住 ESC 进入 bootloader

---

# Obey65 三模开发笔记 (Sketchbook)

## 当前开发状态 (2026-01-19)

**Phase**: 0 - 现有架构清理
**状态**: 文档准备完成，待清理代码恢复编译

### 关键发现

1. **无线库是私有的**: 项目依赖未公开的 wireless 子模块 (libXXX.a)
2. **已有部分实现**: 本地 wireless/ 目录有 BLE HID 的初步实现
3. **编译问题**: 当前配置同时启用了 BLE 和 ESB，但缺少完整实现

### 待解决问题

- [ ] 修复 Python 依赖: `pip3 install click cbor2 intelhex`
- [ ] 禁用 BLE/ESB 恢复有线编译
- [ ] 验证双灯带 RGB 正常工作

### 重要文件位置

| 用途 | 路径 |
|------|------|
| 键盘配置 | `qmk_porting/keyboards/obey65/` |
| 双灯带驱动 | `obey65/ws2812_tmr1.c`, `ws2812_tmr2.c` |
| 无线代码 (WIP) | `obey65/wireless/` |
| 协议接口 | `qmk_porting/protocol/protocol.h` |
| BLE SDK | `sdk/BLE_LIB/` |

### 技术笔记

**ch582_interface_t 接口**:
```c
typedef struct _ch582_interface_t {
    host_driver_t ch582_common_driver;  // 键盘报告发送
    ch582_driver_t ch582_platform_initialize;  // 平台初始化
    ch582_driver_t ch582_protocol_setup;  // 协议设置
    ch582_driver_t ch582_protocol_init;   // 协议初始化
    ch582_driver_t ch582_platform_run;    // 主循环
    // ...
} ch582_interface_t;
```

**双灯带时序** (40MHz):
- T1H = 32 cycles (0.8us)
- T0H = 16 cycles (0.4us)
- Period = 50 cycles (1.25us)
- Reset = 80 bits (100us)

### 下一步行动

1. 执行 Phase 0 清理任务
2. 确保有线模式稳定
3. 开始 Phase 1 基础设施

---

## 文档索引

- 分析报告: [docs/analysis/project-analysis.md](docs/analysis/project-analysis.md)
- 开发路线图: [docs/roadmap/development-roadmap.md](docs/roadmap/development-roadmap.md)
- 进度跟踪: [docs/progress/PROGRESS.md](docs/progress/PROGRESS.md)
- 技术方案:
  - [调试系统](docs/tech/debug-system.md)
  - [蓝牙HID](docs/tech/bluetooth-hid-design.md)
  - [2.4G协议](docs/tech/2.4g-protocol-design.md)
  - [接收器固件](docs/tech/dongle-firmware-design.md)
  - [电源管理](docs/tech/power-management.md)
