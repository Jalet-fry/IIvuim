@echo off
REM ============================================================================
REM Script to detect Qt compiler type (MinGW or MSVC)
REM ============================================================================

echo ============================================
echo Qt Compiler Detection Tool
echo ============================================
echo.

REM Check if we can find qmake
set "QMAKE_PATH="

REM Method 1: Check environment variable
if defined QTDIR (
    echo [INFO] QTDIR found: %QTDIR%
    if exist "%QTDIR%\bin\qmake.exe" (
        set "QMAKE_PATH=%QTDIR%\bin\qmake.exe"
    )
)

REM Method 2: Try common installation paths
if not defined QMAKE_PATH (
    echo [INFO] Searching for Qt installations...
    
    REM Check for Qt 5.5.1 MSVC 2010
    if exist "C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe" (
        echo [FOUND] Qt 5.5.1 MSVC 2010: C:\Qt\Qt5.5.1\5.5\msvc2010\
    )
    
    REM Check for Qt 5.5.1 MSVC 2013
    if exist "C:\Qt\Qt5.5.1\5.5\msvc2013\bin\qmake.exe" (
        echo [FOUND] Qt 5.5.1 MSVC 2013: C:\Qt\Qt5.5.1\5.5\msvc2013\
    )
    
    REM Check for Qt 5.5.1 MinGW
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\qmake.exe" (
        echo [FOUND] Qt 5.5.1 MinGW: C:\Qt\Qt5.5.1\5.5\mingw492_32\
    )
    
    REM Check for other Qt versions
    for /d %%D in (C:\Qt\*) do (
        if exist "%%D\bin\qmake.exe" (
            echo [FOUND] Qt installation: %%D
        )
    )
)

echo.
echo ============================================
echo Checking current project configuration
echo ============================================
echo.

REM Check Makefile if it exists
if exist "Makefile" (
    echo [INFO] Found Makefile, checking compiler...
    findstr /C:"msvc" Makefile >nul
    if %ERRORLEVEL% EQU 0 (
        echo [DETECTED] Project uses MSVC compiler
        echo.
        echo Details:
        findstr /C:"qmake.exe" Makefile | findstr /C:".exe"
    ) else (
        findstr /C:"mingw" Makefile >nul
        if %ERRORLEVEL% EQU 0 (
            echo [DETECTED] Project uses MinGW compiler
            echo.
            echo Details:
            findstr /C:"qmake.exe" Makefile | findstr /C:".exe"
        ) else (
            echo [UNKNOWN] Cannot determine compiler from Makefile
        )
    )
) else (
    echo [INFO] No Makefile found. Project not configured yet.
)

echo.
echo ============================================
echo Checking for MSVC compilers
echo ============================================
echo.

REM Check for Visual Studio installations
if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\bin\cl.exe" (
    echo [FOUND] Visual Studio 2010 ^(32-bit^)
    echo Path: C:\Program Files\Microsoft Visual Studio 10.0\
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\cl.exe" (
    echo [FOUND] Visual Studio 2010 ^(on 64-bit system^)
    echo Path: C:\Program Files ^(x86^)\Microsoft Visual Studio 10.0\
) else (
    echo [NOT FOUND] Visual Studio 2010
)

if exist "C:\Program Files\Microsoft Visual Studio 9.0\VC\bin\cl.exe" (
    echo [FOUND] Visual Studio 2008
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio 9.0\VC\bin\cl.exe" (
    echo [FOUND] Visual Studio 2008
)

echo.
echo ============================================
echo Checking for MinGW
echo ============================================
echo.

where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 (
    echo [FOUND] MinGW g++ in PATH
    g++ --version | findstr "g++"
) else (
    echo [NOT FOUND] MinGW g++ not in PATH
)

REM Check common MinGW locations
if exist "C:\Qt\Qt5.5.1\Tools\mingw492_32\bin\g++.exe" (
    echo [FOUND] MinGW in Qt installation: C:\Qt\Qt5.5.1\Tools\mingw492_32\
)

if exist "C:\MinGW\bin\g++.exe" (
    echo [FOUND] MinGW standalone: C:\MinGW\
)

echo.
echo ============================================
echo Quick Check Results
echo ============================================
echo.

REM Determine what's available
set HAS_MSVC=0
set HAS_MINGW=0
set HAS_QT_MSVC=0
set HAS_QT_MINGW=0

if exist "C:\Program Files\Microsoft Visual Studio 10.0\VC\bin\cl.exe" set HAS_MSVC=1
if exist "C:\Program Files (x86)\Microsoft Visual Studio 10.0\VC\bin\cl.exe" set HAS_MSVC=1

where g++ >nul 2>&1
if %ERRORLEVEL% EQU 0 set HAS_MINGW=1

if exist "C:\Qt\Qt5.5.1\5.5\msvc2010\bin\qmake.exe" set HAS_QT_MSVC=1
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\qmake.exe" set HAS_QT_MINGW=1

echo Installed components:
if %HAS_MSVC%==1 (
    echo   [+] Visual Studio 2010 compiler
) else (
    echo   [-] Visual Studio 2010 compiler NOT FOUND
)

if %HAS_QT_MSVC%==1 (
    echo   [+] Qt for MSVC 2010
) else (
    echo   [-] Qt for MSVC 2010 NOT FOUND
)

if %HAS_MINGW%==1 (
    echo   [+] MinGW compiler
) else (
    echo   [-] MinGW compiler NOT FOUND
)

if %HAS_QT_MINGW%==1 (
    echo   [+] Qt for MinGW
) else (
    echo   [-] Qt for MinGW NOT FOUND
)

echo.
echo ============================================
echo Recommendations for IceKing project:
echo ============================================
echo.

if %HAS_MSVC%==1 (
    if %HAS_QT_MSVC%==1 (
        echo [GOOD] You have MSVC 2010 and Qt for MSVC 2010
        echo [ACTION] You can build the project! Use build_xp.bat
    ) else (
        echo [WARNING] You have MSVC 2010 but no Qt for MSVC 2010
        echo [ACTION] Install Qt 5.5.1 for MSVC 2010
    )
) else (
    echo [MISSING] Visual Studio 2010 not found
    echo [ACTION] Install Visual Studio 2010 ^(Express or Professional^)
    
    if %HAS_QT_MSVC%==1 (
        echo [NOTE] Qt for MSVC 2010 is installed, you just need VS 2010
    ) else (
        echo [ACTION] Then install Qt 5.5.1 for MSVC 2010
    )
)

echo.
echo ============================================
echo For IceKing project you NEED:
echo   1. Visual Studio 2010 ^(any edition^)
echo   2. Qt 5.5.1 for MSVC 2010 ^(32-bit^)
echo.
echo You CANNOT use MinGW because hexiosupp.lib
echo is compiled with MSVC and is incompatible!
echo ============================================
echo.

pause
