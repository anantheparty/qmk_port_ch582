---
name: cmake-embedded
description: CMake 嵌入式构建系统指南。当任务涉及 CMakeLists.txt 修改、添加源文件、配置编译选项、交叉编译设置时使用。
---

# CMake 嵌入式构建指南

## 项目结构理解

### 主要 CMake 文件
```
qmk_port_ch582/
├── CMakeLists.txt              # 顶层配置
├── qmk_porting/
│   ├── CMakeLists.txt          # QMK 适配层
│   ├── common_features.cmake   # 功能开关 (重要!)
│   └── keyboards/
│       └── CMakeLists.txt      # 键盘定义
└── toolchain.cmake             # 工具链配置
```

## 交叉编译配置

### toolchain.cmake 示例
```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# WCH 工具链路径
set(TOOLCHAIN_PREFIX riscv-none-elf-)
# 或使用 WCH 提供的工具链
# set(TOOLCHAIN_PREFIX /path/to/wch/riscv-gcc/bin/riscv-none-embed-)

set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# RISC-V 编译选项
set(CMAKE_C_FLAGS "-march=rv32imac -mabi=ilp32 -msmall-data-limit=8 -mno-save-restore")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections")

# 链接选项
set(CMAKE_EXE_LINKER_FLAGS "-nostartfiles -Xlinker --gc-sections -Wl,--print-memory-usage")
```

## 功能开关模式

### common_features.cmake 中的模式
```cmake
# 条件启用功能
if(RGB_MATRIX_ENABLE)
    add_definitions(-DRGB_MATRIX_ENABLE)
    list(APPEND QMK_PORTING_SOURCES
        "${QMK_BASE_DIR}/quantum/rgb_matrix/..."
        "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812_spi.c"
    )
endif()

# 驱动选择
if(WS2812_DRIVER STREQUAL "spi")
    add_definitions(-DWS2812_SPI)
    list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812_spi.c")
elseif(WS2812_DRIVER STREQUAL "pwm")
    add_definitions(-DWS2812_PWM)
    list(APPEND QMK_PORTING_SOURCES "${CMAKE_CURRENT_LIST_DIR}/drivers/ws2812_pwm.c")
endif()
```

## 添加新源文件

### 正确方式
```cmake
# 使用 list(APPEND) 添加源文件
list(APPEND QMK_PORTING_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/my_feature.c"
)

# 添加头文件搜索路径
include_directories("${CMAKE_CURRENT_LIST_DIR}/include")

# 添加宏定义
add_definitions(-DMY_FEATURE_ENABLE)
```

### 错误方式 (避免!)
```cmake
# 不要直接设置变量，会覆盖之前的值
set(QMK_PORTING_SOURCES "my_feature.c")  # 错误!

# 不要使用 add_executable 添加单个源文件到已有目标
```

## 调试配置

### 生成调试符号
```cmake
# Debug 模式
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -DDEBUG")

# Release 模式
set(CMAKE_BUILD_TYPE Release)  
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")
```

### 打印变量
```cmake
message(STATUS "QMK_PORTING_SOURCES = ${QMK_PORTING_SOURCES}")
message(STATUS "KEYBOARD = ${KEYBOARD}")
message(WARNING "This is a warning")
message(FATAL_ERROR "Build stopped here")
```

## 常用命令

### 构建
```bash
# 创建构建目录
mkdir build && cd build

# 配置 (指定键盘和 keymap)
cmake -G Ninja \
    -DKEYBOARD=my_keyboard \
    -DKEYMAP=default \
    -DCMAKE_BUILD_TYPE=Release \
    ..

# 构建
ninja

# 清理
ninja clean
rm -rf *
```

### 重新配置
```bash
# 修改 CMake 文件后需要重新配置
cmake ..
# 或完全清理
rm CMakeCache.txt
cmake ..
```

## 生成固件文件

### 输出格式转换
```cmake
# 生成 HEX
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
)

# 生成 BIN
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.bin
)

# 生成 UF2 (需要 uf2conv 工具)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND python3 uf2conv.py ${PROJECT_NAME}.bin -o ${PROJECT_NAME}.uf2
)
```

## 常见问题

### 找不到头文件
```cmake
# 添加包含目录
include_directories(
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${SDK_PATH}/HAL/include
)
```

### 链接错误: undefined reference
```cmake
# 确保源文件被添加到编译
list(APPEND SOURCES "missing_file.c")

# 或检查链接库
target_link_libraries(${PROJECT_NAME} -lm)
```

### 编译选项不生效
```cmake
# 检查变量是否被覆盖
# 使用 APPEND 而不是 set
string(APPEND CMAKE_C_FLAGS " -DNEW_FLAG")
```
