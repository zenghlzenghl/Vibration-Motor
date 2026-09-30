@echo off
:: ==============================================
:: C51 项目构建脚本 - 马达振动控制器
:: 功能: 编译源文件并链接生成可烧录的hex文件
:: 环境: Keil C51 编译器
::
:: 用法:
::   build.bat       - 完整编译和链接
::   build.bat clean - 清理编译产物(保留*.hex)
:: ==============================================

:: 设置控制台编码为UTF-8
chcp 65001 >nul

:: ==============================================
:: 编译器路径配置
:: ==============================================
set C51=E:\keil\C51\BIN\c51.exe
set BL51=E:\keil\C51\BIN\bl51.exe
set OH51=E:\keil\C51\BIN\oh51.exe
set C51INC=E:\keil\C51\INC;E:\keil\C51\INC\Atmel;source
set OUTDIR=output

:: ==============================================
:: 命令行参数处理
:: ==============================================
if "%1"=="clean" goto Clean
if "%1"=="" goto Build

echo [ERROR] 未知参数: %1
echo [USAGE] build.bat [clean]
exit /b 1

:: ==============================================
:: Clean 命令 - 清理编译产物（保留*.hex）
:: ==============================================
:Clean
echo [INFO] 执行清理操作...
echo.

:: 创建输出目录（如果不存在）
if not exist %OUTDIR% (
    echo [INFO] 创建输出目录: %OUTDIR%
    mkdir %OUTDIR%
)

:: 删除output目录中的中间文件（保留*.hex）
echo [CLEAN] 删除目标文件(*.obj)...
del /q %OUTDIR%\*.obj 2>nul

echo [CLEAN] 删除绝对目标文件(*.abs)...
del /q %OUTDIR%\*.abs 2>nul

echo [CLEAN] 删除链接映射文件(*.M51)...
del /q %OUTDIR%\*.M51 2>nul

echo [CLEAN] 删除列表文件(*.lst)...
del /q %OUTDIR%\*.lst 2>nul

:: 删除source目录中的临时文件
echo [CLEAN] 删除source目录临时文件...
del /q source\*.lst source\*.OBJ 2>nul

echo.
echo [SUCCESS] 清理完成!
echo [INFO] 已保留文件:

:: 列出保留的hex文件
if exist %OUTDIR%\*.hex (
    dir /b %OUTDIR%\*.hex
) else (
    echo [INFO] (无hex文件)
)

echo.
exit /b 0

:: ==============================================
:: Build 命令 - 完整编译和链接
:: ==============================================
:Build
echo [INFO] 初始化编译器环境...
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
del /q %OUTDIR%\*.abs %OUTDIR%\*.M51 %OUTDIR%\*.obj %OUTDIR%\*.lst 2>nul
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

echo [COMPILE] app.c
%C51% source\app.c LARGE OBJECT(%OUTDIR%\app.obj)

:: 移动列表文件到输出目录
move source\*.lst %OUTDIR% 2>nul
del /q source\*.OBJ 2>nul
echo [INFO] 编译完成，共7个源文件
echo.

:: ==============================================
:: 链接目标文件
:: ==============================================
echo [INFO] 开始链接目标文件...
cd %OUTDIR%

echo [LINK] 正在生成motor.abs...
%BL51% button.obj,gpio.obj,led.obj,motor.obj,power.obj,main.obj,app.obj TO motor.abs

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