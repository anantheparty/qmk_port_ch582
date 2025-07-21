#!/bin/bash
set -e

# 自动查找 factory hex 文件
HEX_FILE=$(find . -maxdepth 1 -name "obey65_factory_*.hex" | head -n 1)
if [ -z "$HEX_FILE" ]; then
  echo "❌ 未找到 obey65_factory_*.hex 文件"
  exit 1
fi

# 自动查找串口
PORT=$(ls /dev/tty.wchusbserial* /dev/tty.usbserial* 2>/dev/null | head -n 1)
if [ -z "$PORT" ]; then
  echo "❌ 未找到 CH582 串口设备"
  echo "请确认开发板已连接，且驱动已安装（如 WCH CH34x 驱动）"
  exit 1
fi

# 固定参数
CHIP="CH582"
BAUD=115200

# 开始烧录
echo "✅ 正在烧录: $HEX_FILE"
echo "📡 使用串口: $PORT"

WCHISPTool_CMD -p "$PORT" -c "$CHIP" -f "$HEX_FILE" -a 0 -b "$BAUD" -d
