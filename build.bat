@echo off
chcp 65001 >nul
echo ==========================================
echo    智能自动输入工具 - 编译脚本
echo ==========================================
echo.

set SOURCE=main.c
set OUTPUT=AutoInputTool.exe
set ICON=app.ico

echo [1/3] 检查编译环境...
where gcc >nul 2>nul
if %errorlevel% neq 0 (
    echo [错误] 未找到 GCC 编译器!
    echo 请安装 MinGW-w64 或 MSYS2 并添加到系统 PATH
    pause
    exit /b 1
)
echo [OK] 找到 GCC 编译器
echo.

echo [2/3] 编译程序...
echo 编译选项: -O2 优化, -mwindows GUI 模式, -s 去除符号表
echo.

gcc -O2 -s -mwindows -municode -o %OUTPUT% %SOURCE% -luser32 -lkernel32 -lcomctl32 -lgdi32 -lmsvcrt

if %errorlevel% neq 0 (
    echo.
    echo [错误] 编译失败!
    pause
    exit /b 1
)

echo.
echo [OK] 编译成功!
echo.

echo [3/3] 验证输出文件...
if exist %OUTPUT% (
    echo [OK] 可执行文件已生成: %OUTPUT%
    for %%F in (%OUTPUT%) do (
        echo     文件大小: %%~zF 字节
        echo     完整路径: %%~fF
    )
) else (
    echo [错误] 未找到输出文件!
    pause
    exit /b 1
)

echo.
echo ==========================================
echo    编译完成!
echo ==========================================
echo.
echo 使用说明:
echo   1. 双击运行 %OUTPUT%
echo   2. 在文本框中输入要自动输入的内容
echo   3. 点击"开始输入"或按 F5 键
echo   4. 程序会在 1 秒后开始自动输入
echo   5. 按 ESC 键可随时停止输入
echo.
echo 支持功能:
echo   - 中英文混合输入
echo   - 自动识别回车换行
echo   - 可调节输入速度
echo   - 实时进度显示
echo   - 全局热键控制
echo.
pause
