@echo off
chcp 65001 > nul
echo.
echo ╔══════════════════════════════════════════════════════════╗
echo ║  Копирование анимаций в папку сборки                     ║
echo ╚══════════════════════════════════════════════════════════╝
echo.

REM Путь к папке сборки Release
set "BUILD_DIR=C:\QT_projects\build-IIvuim-Desktop_Qt_5_5_1_MinGW_32bit-Release\release"

REM Путь к исходной папке анимаций
set "ANIM_SOURCE=Animation"

REM Путь к исходной папке backgrounds
set "BG_SOURCE=backgrounds"

echo Проверяем существование папки сборки...
if not exist "%BUILD_DIR%" (
    echo ❌ ОШИБКА: Папка сборки не найдена!
    echo    %BUILD_DIR%
    echo.
    echo Сначала скомпилируйте проект в Release режиме.
    pause
    exit /b 1
)

echo ✅ Папка сборки найдена: %BUILD_DIR%
echo.

REM Копируем папку Animation
echo Копирую папку Animation...
if exist "%ANIM_SOURCE%" (
    xcopy /E /I /Y "%ANIM_SOURCE%" "%BUILD_DIR%\Animation\"
    echo ✅ Папка Animation скопирована
) else (
    echo ❌ Папка Animation не найдена!
)

echo.

REM Копируем папку backgrounds
echo Копирую папку backgrounds...
if exist "%BG_SOURCE%" (
    xcopy /E /I /Y "%BG_SOURCE%" "%BUILD_DIR%\backgrounds\"
    echo ✅ Папка backgrounds скопирована
) else (
    echo ❌ Папка backgrounds не найдена!
)

echo.
echo ═══════════════════════════════════════════════════════════
echo.
echo ✅ ГОТОВО! Папки скопированы в:
echo    %BUILD_DIR%
echo.
echo Теперь можно запускать:
echo    %BUILD_DIR%\SimpleLabsMenu.exe
echo.
echo ═══════════════════════════════════════════════════════════
pause

