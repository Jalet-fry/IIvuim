@echo off
chcp 65001 >nul
color 0B
cls

echo ╔═══════════════════════════════════════════════════════════════╗
echo ║    ПРОВЕРКА ДАННЫХ НАКОПИТЕЛЕЙ - СРАВНЕНИЕ С ПРОГРАММОЙ      ║
echo ║                        Lab3 - версия 2.0                      ║
echo ╚═══════════════════════════════════════════════════════════════╝
echo.
echo Получение данных через WMI...
echo.

echo ═══════════════════════════════════════════════════════════════
echo   ФИЗИЧЕСКИЕ ДИСКИ (Win32_DiskDrive)
echo ═══════════════════════════════════════════════════════════════
echo.

wmic diskdrive get Model,Manufacturer,SerialNumber,FirmwareRevision,InterfaceType,MediaType,Size /format:list

echo.
echo ═══════════════════════════════════════════════════════════════
echo   ЛОГИЧЕСКИЕ ДИСКИ (свободное место)
echo ═══════════════════════════════════════════════════════════════
echo.

wmic logicaldisk get DeviceID,VolumeName,Size,FreeSpace /format:table

echo.
echo ═══════════════════════════════════════════════════════════════
echo   ПОДРОБНАЯ ИНФОРМАЦИЯ О ДИСКАХ
echo ═══════════════════════════════════════════════════════════════
echo.

for /f "skip=1 tokens=*" %%a in ('wmic diskdrive get Model^,Size^,SerialNumber^,FirmwareRevision') do (
    if not "%%a"=="" (
        echo Диск: %%a
        echo.
    )
)

echo.
echo ╔═══════════════════════════════════════════════════════════════╗
echo ║  ✅ ПРОВЕРКА ЗАВЕРШЕНА!                                      ║
echo ║                                                               ║
echo ║  Сравните эти данные с выводом программы Lab3:               ║
echo ║                                                               ║
echo ║  Примечания:                                                  ║
echo ║  - InterfaceType может показывать SCSI для NVMe дисков       ║
echo ║  - Manufacturer может быть пустым или (Standard disk drives) ║
echo ║  - Программа Lab3 исправляет эти значения автоматически!     ║
echo ║                                                               ║
echo ║  💡 Для более подробной информации используйте:              ║
echo ║     check_disk_info.ps1 (PowerShell скрипт)                  ║
echo ╚═══════════════════════════════════════════════════════════════╝
echo.

pause

