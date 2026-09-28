@echo off
:: ==============================================
:: C51 项目构建脚本 - 时钟LCM显示项目
:: 功能: 编译源文件并链接生成可烧录的hex文件
:: 环境: Keil C51 编译器
:: ==============================================

:: 设置控制台编码为UTF-8
chcp 65001 >nul

:: ==============================================
:: 编译器路径配置
:: ==============================================
echo [INFO] 初始化编译器环境...
set C51=E:\keil\C51\BIN\c51.exe
set BL51=E:\keil\C51\BIN\bl51.exe
set OH51=E:\keil\C51\BIN\oh51.exe
set C51INC=E:\keil\C51\INC;E:\keil\C51\INC\Atmel;source
set OUTDIR=output

echo [INFO] 编译器路径: %C51%
echo [INFO] 输出目录: %OUTDIR%
echo.

:: ==============================================
:: 清理旧文件
:: ==============================================
echo [INFO] 清理旧的编译产物...

:: 创建输出目录（如果不存在）
if not exist %OUTDIR% (
    echo [INFO] 创建输出目录: %OUTDIR%
    mkdir %OUTDIR%
)

:: 删除输出目录中的旧文件
del /q %OUTDIR%\*.hex %OUTDIR%\*.abs %OUTDIR%\*.M51 %OUTDIR%\*.obj %OUTDIR%\*.lst 2>nul
echo [INFO] 已清理输出目录

:: 删除source目录中的临时文件
del /q source\*.lst source\*.OBJ 2>nul
echo [INFO] 已清理source目录临时文件
echo.

:: ==============================================
:: 编译源文件
:: ==============================================
echo [INFO] 开始编译源文件...

echo [COMPILE] button.c
%C51% source\button.c LARGE OBJECT(%OUTDIR%\button.obj)

echo [COMPILE] gpio.c
%C51% source\gpio.c LARGE OBJECT(%OUTDIR%\gpio.obj)

echo [COMPILE] led.c
%C51% source\led.c LARGE OBJECT(%OUTDIR%\led.obj)

echo [COMPILE] motor.c
%C51% source\motor.c LARGE OBJECT(%OUTDIR%\motor.obj)

echo [COMPILE] power.c
%C51% source\power.c LARGE OBJECT(%OUTDIR%\power.obj)

echo [COMPILE] main.c
%C51% source\main.c LARGE OBJECT(%OUTDIR%\main.obj)

:: 移动列表文件到输出目录
move source\*.lst %OUTDIR% 2>nul
del /q source\*.OBJ 2>nul
echo [INFO] 编译完成，共6个源文件
echo.

:: ==============================================
:: 链接目标文件
:: ==============================================
echo [INFO] 开始链接目标文件...
cd %OUTDIR%

echo [LINK] 正在生成motor.abs...
%BL51% button.obj,gpio.obj,led.obj,motor.obj,power.obj,main.obj TO motor.abs

echo [OH51] 正在生成hex文件...
%OH51% motor.abs

:: 统一hex文件名（转为小写）
ren MOTOR.hex motor.hex 2>nul

cd ..
echo [INFO] 链接完成
echo.

:: ==============================================
:: 构建结果检查
:: ==============================================
if exist %OUTDIR%\motor.hex (
    echo [SUCCESS] 构建成功!
    echo [SUCCESS] 输出文件: %OUTDIR%\motor.hex
    echo.
    :: 显示生成文件的基本信息
    dir %OUTDIR%\motor.hex
) else (
    echo [ERROR] 构建失败!
    echo [ERROR] 未找到 %OUTDIR%\motor.hex
    exit /b 1
)