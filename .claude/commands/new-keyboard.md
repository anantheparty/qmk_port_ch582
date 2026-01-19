---
description: 创建新键盘定义的脚手架。用法：/new-keyboard <name> <rows> <cols>
---

# 创建新键盘

为新键盘创建基本文件结构。

## 需要的参数
- **name**: 键盘名称 (小写，无空格)
- **rows**: 矩阵行数
- **cols**: 矩阵列数

## 将创建的文件
```
qmk_porting/keyboards/<name>/
├── config.h          # 硬件配置
├── halconf.h         # HAL 配置
├── mcuconf.h         # MCU 配置
├── <name>.c          # 键盘初始化
├── <name>.h          # LAYOUT 宏定义
├── info.json         # 键盘元数据
└── keymaps/
    └── default/
        └── keymap.c  # 默认键位
```

## 执行步骤
1. 询问 LED 数量 (RGB Matrix)
2. 询问是否支持 VIA
3. 询问连接模式 (有线/蓝牙/三模)
4. 生成所有模板文件
5. 提示需要手动配置的引脚

## 配置模板

### config.h 要点
```c
#define MATRIX_ROWS {{rows}}
#define MATRIX_COLS {{cols}}
#define MATRIX_ROW_PINS { /* 需要配置 */ }
#define MATRIX_COL_PINS { /* 需要配置 */ }
```

创建后提醒用户配置实际的 GPIO 引脚。
