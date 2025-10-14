@echo off
chcp 65001 > nul

REM Этот скрипт запускается из КОРНЯ проекта IIvuim
echo.
echo ╔════════════════════════════════════════════════════════════╗
echo ║     Проверка наличия анимаций для IIvuim проекта           ║
echo ╚════════════════════════════════════════════════════════════╝
echo.
echo Рабочая директория: %CD%
echo.

set "MISSING=0"
set "FOUND=0"

echo Проверяем основные анимации Jake...
echo.

call :check "Animation\Jake.gif" "Базовая анимация Jake"
call :check "Animation\Jake dance.gif" "Танец Jake"
call :check "Animation\Jake laugh.gif" "Смех Jake"
call :check "Animation\Jake scate.gif" "Катание Jake"
call :check "Animation\Jake vig eyes.gif" "Моргание Jake"
call :check "Animation\jake with cicrle.gif" "Jake с кружком"
call :check "Animation\Jake_picture.jpg" "Статичное изображение Jake"

echo.
echo Проверяем Lab5 анимации...
echo.

call :check "Animation\004.gif" "Lab5 - извлечение USB"
call :check "Animation\005.gif" "Lab5 - подключение USB"

echo.
echo Проверяем пронумерованные анимации (001-020)...
echo.

for %%i in (001 002 003 004 005 006 007 008 009 010 011 012 013 014 015 016 017 018 019 020) do (
    call :check "Animation\%%i.gif" "Анимация %%i"
)

echo.
echo Проверяем assets...
echo.
call :check "Animation\assets\jake_base.gif" "Jake базовая (assets)"

echo.
echo ════════════════════════════════════════════════════════════
echo.
echo ✅ Найдено файлов: %FOUND%
echo ❌ Отсутствует:     %MISSING%
echo.

if %MISSING% GTR 0 (
    echo ⚠️  ВНИМАНИЕ: Некоторые файлы отсутствуют!
    echo    Приложение может работать некорректно.
    echo.
    echo 📝 Рекомендации:
    echo    1. Восстановите отсутствующие файлы из резервной копии
    echo    2. Или закомментируйте использование этих анимаций в коде
    echo    3. Минимально необходимые:
    echo       - Animation\Jake.gif (для главного меню^)
    echo       - Animation\004.gif и 005.gif (для Lab5^)
) else (
    echo ✅ Все анимации на месте! Можно компилировать проект.
    echo.
    echo 📝 Следующие шаги:
    echo    1. В Qt Creator: Projects -^> Run Settings
    echo    2. Working directory: %%{sourceDir} или %CD%
    echo    3. Build -^> Rebuild All
    echo    4. Запустить приложение
)

echo.
echo ════════════════════════════════════════════════════════════
pause
goto :eof

:check
if exist "%~1" (
    echo ✅ %~2
    set /a FOUND+=1
) else (
    echo ❌ %~2 - ОТСУТСТВУЕТ: %~1
    set /a MISSING+=1
)
goto :eof

