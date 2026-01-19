---
name: embedded-debugging
description: 嵌入式调试和问题排查指南。当遇到固件不工作、键盘无响应、USB 枚举失败、蓝牙连接问题时使用。
---

# 嵌入式调试指南

## 调试工具

### 硬件工具
- **WCH-Link**: 官方调试器，支持 SWD/JTAG
- **逻辑分析仪**: 分析 SPI/I2C/UART 信号
- **万用表**: 检查电压和连通性

### 软件工具
- **MounRiver Studio**: WCH 官方 IDE
- **OpenOCD**: 开源调试器 (需要 WCH 补丁)
- **串口终端**: minicom, PuTTY, screen

## 串口调试

### 启用 UART 日志
```c
// 在 config.h 中
#define UART_DEBUG_ENABLE
#define DEBUG_UART UART1

// 初始化代码
void debug_init(void) {
    GPIOB_ModeCfg(GPIO_Pin_7, GPIO_ModeIN_PU);   // RX
    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA); // TX
    UART1_DefInit();
    UART1_BaudRateCfg(115200);
}

// 打印调试信息
#define DEBUG_PRINT(fmt, ...) printf("[DBG] " fmt "\r\n", ##__VA_ARGS__)
```

### 重定向 printf
```c
int _write(int fd, char *buf, int size) {
    for (int i = 0; i < size; i++) {
        while (R8_UART1_TFC >= UART_FIFO_SIZE);
        R8_UART1_THR = buf[i];
    }
    return size;
}
```

## 常见问题排查

### 1. 键盘完全无响应

**检查清单**:
- [ ] 供电电压正确 (3.3V)
- [ ] 晶振起振 (示波器检查)
- [ ] Reset 引脚状态
- [ ] Boot 引脚状态 (应为高电平)

**代码检查**:
```c
// 最简测试：LED 闪烁
int main(void) {
    SetSysClock(CLK_SOURCE_PLL_60MHz);
    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);
    while(1) {
        GPIOB_InverseBits(GPIO_Pin_4);
        mDelaymS(500);
    }
}
```

### 2. USB 枚举失败

**症状**: 电脑不识别设备

**检查步骤**:
```bash
# Linux 查看 USB 日志
dmesg -w | grep -i usb

# macOS
log stream --predicate 'subsystem == "com.apple.usb"'
```

**常见原因**:
1. USB D+/D- 接反
2. USB 描述符错误
3. 端点配置不正确

**调试代码**:
```c
// 在 USB 回调中添加日志
void USB_IRQHandler(void) {
    DEBUG_PRINT("USB INT: 0x%02X", R8_USB_INT_FG);
    // ...
}
```

### 3. 矩阵扫描问题

**症状**: 部分按键不响应或触发错误

**检查方法**:
```c
// 打印矩阵状态
void matrix_debug(void) {
    matrix_scan();
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        DEBUG_PRINT("Row %d: 0x%04X", row, matrix_get_row(row));
    }
}
```

**硬件检查**:
- 二极管方向是否正确
- 焊接是否虚焊
- 引脚是否配置正确 (上拉/下拉)

### 4. RGB 灯不亮

**WS2812 SPI 模式检查**:
```c
// 检查 SPI 时钟频率 (应为 ~6.4MHz for WS2812)
// 时序要求: T0H=350ns, T0L=800ns, T1H=700ns, T1L=600ns

// 用逻辑分析仪检查 MOSI 引脚波形
void ws2812_debug(void) {
    uint8_t test_data[] = {0xFF, 0x00, 0x00};  // 红色
    ws2812_setleds(test_data, 1);
}
```

### 5. 蓝牙连接失败

**检查清单**:
- [ ] 天线连接正确
- [ ] BLE 库正确链接
- [ ] 广播参数配置
- [ ] 配对信息清除后重试

**调试代码**:
```c
// BLE 状态回调
void BLE_StateCallback(uint8_t state) {
    switch(state) {
        case GAPROLE_STARTED:
            DEBUG_PRINT("BLE Started");
            break;
        case GAPROLE_ADVERTISING:
            DEBUG_PRINT("Advertising...");
            break;
        case GAPROLE_CONNECTED:
            DEBUG_PRINT("Connected!");
            break;
        case GAPROLE_DISCONNECTED:
            DEBUG_PRINT("Disconnected");
            break;
    }
}
```

## GDB 调试

### OpenOCD 配置
```bash
# openocd.cfg
source [find interface/wch-link.cfg]
transport select swd
source [find target/wch-riscv.cfg]

# 启动 OpenOCD
openocd -f openocd.cfg
```

### GDB 命令
```gdb
# 连接
target remote localhost:3333

# 加载固件
load firmware.elf

# 断点
break main
break matrix_scan

# 运行
continue

# 查看寄存器
info registers

# 查看内存
x/10x 0x20000000

# 单步执行
step
next
```

## 性能分析

### 测量扫描时间
```c
void profile_matrix_scan(void) {
    uint32_t start = SysTick->CNT;
    matrix_scan();
    uint32_t end = SysTick->CNT;
    DEBUG_PRINT("Scan time: %d cycles", end - start);
}
```

### 检查 RAM 使用
```bash
# 使用 size 工具
riscv-none-elf-size firmware.elf
   text    data     bss     dec     hex filename
  45632    1024    8192   54848    d640 firmware.elf

# RAM = data + bss
# Flash = text + data
```

## 安全提示

### 避免变砖
1. **保留 Bootloader**: 不要擦除 boot 区域
2. **Bootmagic Lite**: 确保有进入 bootloader 的方法
3. **备份**: 刷固件前备份当前固件
4. **测试**: 新固件先在开发板上测试

### 恢复方法
```bash
# 使用 WCH-Link 强制进入 ISP 模式
# 1. 按住 BOOT 按钮
# 2. 按下 RESET
# 3. 松开 RESET
# 4. 松开 BOOT
# 5. 使用 wchisp 工具烧录
wchisp flash firmware.bin
```
