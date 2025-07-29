# Obey65 键盘

这是一个基于CH582微控制器的65%机械键盘配置。

## 硬件规格

- **微控制器**: CH582
- **矩阵**: 5行 x 15列
- **布局**: 65% ANSI布局
- **连接方式**: USB + 蓝牙
- **RGB灯光**: 4颗 WS2812 LED，位于键盘右侧

## 引脚配置

### 行引脚 (Row Pins)
- Row 0: PA8
- Row 1: PA9  
- Row 2: PB9
- Row 3: PB13
- Row 4: PA7

### 列引脚 (Column Pins)
- Col 0: PB8
- Col 1: PB16
- Col 2: PB15
- Col 3: PB14
- Col 4: PB7
- Col 5: PB6
- Col 6: PB5
- Col 7: PB4
- Col 8: PB3
- Col 9: PB2
- Col 10: PB1
- Col 11: PB0
- Col 12: PB21
- Col 13: PB20
- Col 14: PB19

### RGB灯光引脚
- **WS2812 数据引脚**: PA11 (TMR2 PWM输出)
- **WS2812 使能引脚**: PA11
- **LED数量**: 4颗
- **驱动方式**: PWM (TMR2定时器)

## 功能特性

- ✅ USB连接
- ✅ 蓝牙连接 (BLE)
- ✅ 2.4G无线连接 (ESB)
- ✅ VIA配置支持
- ✅ 多层键映射
- ✅ NKRO支持
- ✅ RGB灯光 (WS2812)
- ❌ 编码器支持 (暂未启用)

## RGB灯光功能

键盘配备了4颗WS2812 RGB LED，竖着排列在键盘右侧。支持多种灯光效果：

### 支持的灯光效果
- 呼吸效果 (Breathing)
- 螺旋带效果 (Band Spiral)
- 循环全色效果 (Cycle All)
- 左右循环效果 (Cycle Left Right)
- 上下循环效果 (Cycle Up Down)
- 彩虹移动人字形 (Rainbow Moving Chevron)
- 环形循环效果 (Cycle Out In)
- 双环循环效果 (Cycle Out In Dual)
- 风车循环效果 (Cycle Pinwheel)
- 螺旋循环效果 (Cycle Spiral)
- 双信标效果 (Dual Beacon)
- 彩虹信标效果 (Rainbow Beacon)
- 雨滴效果 (Raindrops)
- 软糖雨滴效果 (Jellybean Raindrops)

### RGB控制按键 (Layer 2)
在Layer 2中提供了完整的RGB控制：

- `RGB_TOG`: 开关RGB灯光
- `RGB_MOD`: 切换下一个灯光模式
- `RGB_RMOD`: 切换上一个灯光模式
- `RGB_HUI`: 增加色相
- `RGB_HUD`: 减少色相
- `RGB_SAI`: 增加饱和度
- `RGB_SAD`: 减少饱和度
- `RGB_VAI`: 增加亮度
- `RGB_VAD`: 减少亮度
- `RGB_SPI`: 增加效果速度
- `RGB_SPD`: 减少效果速度

## 构建说明

1. 确保已安装QMK固件开发环境
2. 进入键盘目录: `cd qmk_porting/keyboards/obey65`
3. 构建固件: `qmk compile -kb obey65 -km default`
4. 生成的固件文件将位于 `qmk_firmware/.build/obey65_default.uf2`

## 刷写固件

1. 将键盘进入DFU模式
2. 将生成的 `.uf2` 文件复制到键盘的虚拟磁盘
3. 等待刷写完成，键盘将自动重启

## 键映射

默认键映射包含4层:
- **Layer 0**: 基础QWERTY布局
- **Layer 1**: 功能键层
- **Layer 2**: RGB控制层
- **Layer 3**: 预留层

## 自定义键码

支持以下自定义键码:
- `USB`: 切换到USB模式
- `BLE1-BLE16`: 蓝牙槽位切换
- `BC_A`: 清除所有蓝牙绑定
- `2.4G`: 切换到2.4G模式
- `BAT`: 电池电量指示

## 注意事项

- RGB灯光功能已启用，使用PA11引脚作为WS2812数据线（TMR2 PWM输出）
- 使用PWM驱动方式（TMR2定时器），默认启用多种灯光效果
- 通过按住MO(2)键进入Layer 2来访问RGB控制功能
- 确保二极管方向设置为 `COL2ROW`
- 建议使用VIA软件进行键映射配置

## 许可证

本项目采用GPL v2许可证。 