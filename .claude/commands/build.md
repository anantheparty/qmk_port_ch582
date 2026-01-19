---
description: 构建键盘固件。用法：/build [keyboard_name] [keymap]
---

# 构建固件

请帮我构建键盘固件。

## 步骤
1. 检查当前目录是否在 qmk_port_ch582 项目中
2. 如果没有 build 目录，创建它
3. 运行 cmake 配置（如果需要）
4. 运行 ninja 构建
5. 报告构建结果和固件位置

## 如果指定了参数
- 第一个参数是键盘名称 (KEYBOARD)
- 第二个参数是 keymap 名称 (KEYMAP)

## 构建命令参考
```bash
cd build
cmake -G Ninja -DKEYBOARD={{keyboard}} -DKEYMAP={{keymap}} ..
ninja
```

## 构建成功后
- 显示固件文件位置 (.hex, .bin, .uf2)
- 显示固件大小 (Flash/RAM 使用)
