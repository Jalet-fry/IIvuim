@echo off
REM ============================================================================
REM Build script for IceKing project - Windows XP compatible version
REM Requires: Visual Studio 2010 + Qt 5.5.1 MSVC 2010
REM ============================================================================

echo ============================================
echo IceKing Project Builder for Windows XP
echo ============================================
echo.

REM Check if running in VS Command Prompt
where cl >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] MSVC compiler not found!
    echo.
    echo Please run this script from "Visual Studio Command Prompt 2010"
    echo Path: Start Menu -^> Microsoft Visual Studio 2010 -^> Visual Studio Tools
    echo       -^> Visual Studio Command Prompt ^(2010^)
    echo.
    pause
    exit /b 1
)

echo [OK] MSVC compiler found
cl 2>&1 | findstr "Version" | findstr "16"
if %ERRORLEVEL% EQU 0 (
    echo [OK] Using Visual Studio 2010 compiler
) else (
    echo [WARNING] Compiler version might not be VS 2010!
    echo [WARNING] Expected version 16.x
)
echo.

REM Qt paths
set QT_DIR=C:\Qt\Qt5.5.1\5.5\msvc2010
set QMAKE=%QT_DIR%\bin\qmake.exe
set QT_BIN=%QT_DIR%\bin
set QT_PLUGINS=%QT_DIR%\plugins

REM Check if Qt is installed
if not exist "%QMAKE%" (
    echo [ERROR] Qt 5.5.1 MSVC 2010 not found!
    echo Expected path: %QMAKE%
    echo.
    echo Please install Qt 5.5.1 for MSVC 2010 from:
    echo https://download.qt.io/archive/qt/5.5/5.5.1/
    echo.
    pause
    exit /b 1
)

echo [OK] Qt 5.5.1 MSVC 2010 found
echo [INFO] Qt path: %QT_DIR%
echo.

REM Clean old build files
echo [STEP 1] Cleaning old build files...
if exist Makefile (
    nmake distclean >nul 2>&1
    del /Q Makefile >nul 2>&1
    del /Q Makefile.Debug >nul 2>&1
    del /Q Makefile.Release >nul 2>&1
)
echo [OK] Cleaned
echo.

REM Run qmake
echo [STEP 2] Running qmake...
"%QMAKE%" IceKing.pro "CONFIG+=release"
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] qmake failed!
    pause
    exit /b 1
)
echo [OK] Makefile generated
echo.

REM Build project
echo [STEP 3] Building project with nmake...
nmake release
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] Build failed!
    pause
    exit /b 1
)
echo [OK] Build successful
echo.

REM Check if exe exists
if not exist "release\IceKing.exe" (
    echo [ERROR] IceKing.exe was not created!
    pause
    exit /b 1
)

echo [STEP 4] Copying Qt dependencies...

REM Copy Qt DLLs to release folder
copy /Y "%QT_BIN%\Qt5Core.dll" release\ >nul
copy /Y "%QT_BIN%\Qt5Gui.dll" release\ >nul
copy /Y "%QT_BIN%\Qt5Widgets.dll" release\ >nul

REM Create platforms directory and copy plugin
if not exist "release\platforms" mkdir release\platforms
copy /Y "%QT_PLUGINS%\platforms\qwindows.dll" release\platforms\ >nul

REM Copy MSVC runtime (should already be there from build)
if exist "msvcp100.dll" copy /Y "msvcp100.dll" release\ >nul
if exist "msvcr100.dll" copy /Y "msvcr100.dll" release\ >nul

REM Copy driver
if exist "HexIO.sys" copy /Y "HexIO.sys" release\ >nul

echo [OK] Dependencies copied
echo.

REM Create deployment package
echo [STEP 5] Creating deployment package...
set DEPLOY_DIR=IceKing_WinXP_Deploy
if exist "%DEPLOY_DIR%" rmdir /S /Q "%DEPLOY_DIR%"
mkdir "%DEPLOY_DIR%"

xcopy /Y release\IceKing.exe "%DEPLOY_DIR%\" >nul
xcopy /Y release\*.dll "%DEPLOY_DIR%\" >nul
if exist "HexIO.sys" xcopy /Y HexIO.sys "%DEPLOY_DIR%\" >nul
if exist "release\platforms" xcopy /E /Y release\platforms "%DEPLOY_DIR%\platforms\" >nul
if exist "img" xcopy /E /Y img "%DEPLOY_DIR%\img\" >nul

echo [OK] Deployment package created in: %DEPLOY_DIR%\
echo.

REM Display build info
echo ============================================
echo BUILD COMPLETED SUCCESSFULLY!
echo ============================================
echo.
echo Executable: release\IceKing.exe
echo Deploy folder: %DEPLOY_DIR%\
echo.
echo Files ready for Windows XP:
echo   - IceKing.exe
echo   - Qt5Core.dll, Qt5Gui.dll, Qt5Widgets.dll
echo   - msvcp100.dll, msvcr100.dll
echo   - platforms\qwindows.dll
echo   - HexIO.sys (driver)
echo.
echo Copy the entire "%DEPLOY_DIR%" folder to your Windows XP VM
echo.

REM Create instructions file
echo Creating DEPLOY_README.txt...
(
echo ============================================
echo IceKing for Windows XP - Deployment Package
echo ============================================
echo.
echo Built on: %DATE% %TIME%
echo Compiler: MSVC 2010
echo Qt Version: 5.5.1 MSVC 2010
echo Architecture: 32-bit ^(x86^)
echo.
echo Installation on Windows XP:
echo.
echo 1. Copy this entire folder to Windows XP
echo.
echo 2. Install HexIO driver ^(as Administrator^):
echo    - Copy HexIO.sys to C:\Windows\System32\drivers\
echo    - Open Command Prompt as Administrator
echo    - Run: sc create HexIO binPath= C:\Windows\System32\drivers\HexIO.sys type= kernel start= demand
echo    - Run: sc start HexIO
echo.
echo 3. Run IceKing.exe
echo.
echo Troubleshooting:
echo.
echo - If "MSVCP100.dll not found": Install Microsoft Visual C++ 2010 Redistributable ^(x86^)
echo - If driver fails: Check you have Administrator rights
echo - If Qt platform plugin missing: Make sure platforms\qwindows.dll exists
echo.
echo ============================================
) > "%DEPLOY_DIR%\DEPLOY_README.txt"

echo [OK] Instructions created: %DEPLOY_DIR%\DEPLOY_README.txt
echo.

echo ============================================
echo All done! Ready for Windows XP deployment.
echo ============================================
pause
