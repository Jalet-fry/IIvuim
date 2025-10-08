@echo off
REM Скрипт для сборки проекта на Windows XP с MinGW

echo ===================================
echo Building IIvuim Project
echo ===================================

REM Добавляем Qt и MinGW в PATH
set PATH=C:\Qt\Qt5.5.1\5.5\mingw492_32\bin;C:\Qt\Qt5.5.1\Tools\mingw492_32\bin;%PATH%

REM Проверяем что qmake доступен
echo Checking qmake...
qmake -v
if errorlevel 1 (
    echo ERROR: qmake not found!
    pause
    exit /b 1
)

REM Проверяем что mingw32-make доступен
echo Checking mingw32-make...
mingw32-make --version
if errorlevel 1 (
    echo ERROR: mingw32-make not found!
    pause
    exit /b 1
)

REM Генерируем Makefile
echo.
echo Generating Makefile...
qmake IIvuim.pro -spec win32-g++ "CONFIG+=release"

REM Компилируем проект
echo.
echo Building...
mingw32-make -j4

if errorlevel 1 (
    echo.
    echo ===================================
    echo BUILD FAILED!
    echo ===================================
    pause
    exit /b 1
)

echo.
echo ===================================
echo BUILD SUCCESS!
echo ===================================
echo.
echo Executable: release\SimpleLabsMenu.exe
echo.
pause


