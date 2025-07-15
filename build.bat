@echo off
chcp 65001 >nul
setlocal

REM 设置默认值
set KEYBOARD=m2wired
set KEYMAP=default

REM 如果有传入参数就覆盖默认值
if not "%~1"=="" set KEYBOARD=%~1
if not "%~2"=="" set KEYMAP=%~2

REM 切换到 build 目录
cd /d %~dp0build

REM 清理旧构建内容
echo 清理 build 目录...
del /q /s * >nul 2>nul

REM 配置 CMake
echo 配置 CMake 中：keyboard=%KEYBOARD%, keymap=%KEYMAP%
cmake -Dkeyboard=%KEYBOARD% -Dkeymap=%KEYMAP% .. -G "Unix Makefiles"
if errorlevel 1 (
    echo CMake 配置失败
    pause
    exit /b 1
)

REM 编译
echo 开始编译...
make -j%NUMBER_OF_PROCESSORS%
if errorlevel 1 (
    echo 编译失败
) else (
    echo 编译成功
)

pause
endlocal
