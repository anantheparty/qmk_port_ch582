#!/bin/bash
set -e

# 设置默认值
KEYBOARD="obey65"
KEYMAP="default"

# 如果传入参数，则覆盖默认值
if [ ! -z "$1" ]; then
  KEYBOARD="$1"
fi

if [ ! -z "$2" ]; then
  KEYMAP="$2"
fi

mkdir -p build.old

# 移动旧文件（如果存在）
if ls obey65_factory_*.hex 1> /dev/null 2>&1; then
  mv obey65_factory_*.hex build.old/
fi

if ls obey65_upgrade_*.uf2 1> /dev/null 2>&1; then
  mv obey65_upgrade_*.uf2 build.old/
fi

# 进入 build 目录
if [ ! -d "$(dirname "$0")/build" ]; then
  mkdir -p "$(dirname "$0")/build"
fi
cd "$(dirname "$0")/build"

# 清理旧构建内容
echo "清理 build 目录..."
rm -rf ./*

# 配置 CMake
echo "配置 CMake 中：keyboard=${KEYBOARD}, keymap=${KEYMAP}"
cmake -Dkeyboard="${KEYBOARD}" -Dkeymap="${KEYMAP}" .. -G "Unix Makefiles"

# 编译
echo "开始编译..."
if make -j"$(nproc)"; then
  echo "编译成功"
else
  echo "编译失败"
fi
