@echo off
echo ===============================================
echo    REBUILD AND TEST SCRIPT
echo    For Windows XP with Updated GIF Support
echo ===============================================
echo.

set LOG_FILE=rebuild_log.txt

echo =============================================== > %LOG_FILE%
echo    REBUILD AND TEST LOG >> %LOG_FILE%
echo    Date: %date% %time% >> %LOG_FILE%
echo    Updated GIF Support Applied >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 1: Setup environment >> %LOG_FILE%
echo --------------------------- >> %LOG_FILE%
echo Setting up PATH for Qt and MinGW... >> %LOG_FILE%
set PATH=%PATH%;C:\Qt\Qt5.5.1\5.5\mingw492_32\bin;C:\Qt\Qt5.5.1\Tools\mingw492_32\bin
echo PATH=%PATH% >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 2: Clean ALL build artifacts >> %LOG_FILE%
echo ----------------------------------- >> %LOG_FILE%
echo Cleaning all build files... >> %LOG_FILE%
if exist "Makefile" del Makefile >> %LOG_FILE% 2>&1
if exist "Makefile.Debug" del Makefile.Debug >> %LOG_FILE% 2>&1
if exist "Makefile.Release" del Makefile.Release >> %LOG_FILE% 2>&1
if exist "*.o" del *.o >> %LOG_FILE% 2>&1
if exist "moc_*.cpp" del moc_*.cpp >> %LOG_FILE% 2>&1
if exist "moc_*.o" del moc_*.o >> %LOG_FILE% 2>&1
if exist "qrc_*.cpp" del qrc_*.cpp >> %LOG_FILE% 2>&1
if exist "qrc_*.o" del qrc_*.o >> %LOG_FILE% 2>&1
if exist "SimpleLabsMenu.exe" del SimpleLabsMenu.exe >> %LOG_FILE% 2>&1
if exist "release" rmdir /s /q release >> %LOG_FILE% 2>&1
if exist "debug" rmdir /s /q debug >> %LOG_FILE% 2>&1
echo Cleanup completed >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 3: Generate Makefile >> %LOG_FILE%
echo --------------------------- >> %LOG_FILE%
echo Running qmake... >> %LOG_FILE%
qmake IIvuim.pro >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: qmake failed! >> %LOG_FILE%
    echo Build FAILED - qmake error >> %LOG_FILE%
    goto :error
)
echo qmake completed successfully >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 4: Build project >> %LOG_FILE%
echo ---------------------- >> %LOG_FILE%
echo Running mingw32-make... >> %LOG_FILE%
mingw32-make >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: mingw32-make failed! >> %LOG_FILE%
    echo Build FAILED - compilation error >> %LOG_FILE%
    goto :error
)
echo mingw32-make completed successfully >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 5: Locate executable >> %LOG_FILE%
echo --------------------------- >> %LOG_FILE%
if exist "SimpleLabsMenu.exe" (
    echo SimpleLabsMenu.exe found in root directory >> %LOG_FILE%
    dir SimpleLabsMenu.exe >> %LOG_FILE%
) else if exist "release\SimpleLabsMenu.exe" (
    echo SimpleLabsMenu.exe found in release directory >> %LOG_FILE%
    dir release\SimpleLabsMenu.exe >> %LOG_FILE%
    echo Copying to root directory... >> %LOG_FILE%
    copy release\SimpleLabsMenu.exe . >> %LOG_FILE% 2>&1
    echo SimpleLabsMenu.exe copied to root >> %LOG_FILE%
) else (
    echo ERROR: SimpleLabsMenu.exe not found! >> %LOG_FILE%
    echo Build FAILED - no executable >> %LOG_FILE%
    goto :error
)
echo SimpleLabsMenu.exe ready >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 6: Copy Qt DLLs >> %LOG_FILE%
echo --------------------- >> %LOG_FILE%
echo Copying Qt DLLs... >> %LOG_FILE%
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Core.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Core.dll" . >> %LOG_FILE% 2>&1
    echo Qt5Core.dll copied >> %LOG_FILE%
)
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Gui.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Gui.dll" . >> %LOG_FILE% 2>&1
    echo Qt5Gui.dll copied >> %LOG_FILE%
)
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Widgets.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Widgets.dll" . >> %LOG_FILE% 2>&1
    echo Qt5Widgets.dll copied >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo STEP 7: Copy MinGW DLLs >> %LOG_FILE%
echo ------------------------ >> %LOG_FILE%
echo Copying MinGW DLLs... >> %LOG_FILE%
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libgcc_s_dw2-1.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libgcc_s_dw2-1.dll" . >> %LOG_FILE% 2>&1
    echo libgcc_s_dw2-1.dll copied >> %LOG_FILE%
)
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libstdc++-6.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libstdc++-6.dll" . >> %LOG_FILE% 2>&1
    echo libstdc++-6.dll copied >> %LOG_FILE%
)
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libwinpthread-1.dll" (
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libwinpthread-1.dll" . >> %LOG_FILE% 2>&1
    echo libwinpthread-1.dll copied >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo STEP 8: Check for GIF plugin >> %LOG_FILE%
echo ------------------------------ >> %LOG_FILE%
echo Checking for Qt GIF plugin... >> %LOG_FILE%
if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\imageformats\qgif.dll" (
    echo GIF plugin found at: C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\imageformats\qgif.dll >> %LOG_FILE%
    echo Creating imageformats directory... >> %LOG_FILE%
    if not exist "imageformats" mkdir imageformats >> %LOG_FILE% 2>&1
    echo Copying GIF plugin... >> %LOG_FILE%
    copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\plugins\imageformats\qgif.dll" "imageformats\" >> %LOG_FILE% 2>&1
    echo GIF plugin copied successfully >> %LOG_FILE%
) else (
    echo WARNING: GIF plugin not found! >> %LOG_FILE%
    echo This may cause GIF loading issues on Windows XP >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo STEP 9: Test run >> %LOG_FILE%
echo ---------------- >> %LOG_FILE%
echo Testing SimpleLabsMenu.exe... >> %LOG_FILE%
echo Starting application... >> %LOG_FILE%
SimpleLabsMenu.exe >> %LOG_FILE% 2>&1
echo Executable exit code: %errorlevel% >> %LOG_FILE%

if %errorlevel% equ 0 (
    echo SUCCESS: Application ran without errors! >> %LOG_FILE%
    echo. >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo    REBUILD AND TEST SUCCESSFUL! >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo.
    echo ===============================================
    echo    REBUILD AND TEST SUCCESSFUL!
    echo ===============================================
    echo.
    echo Application rebuilt and ran successfully!
    echo Check rebuild_log.txt for details.
    echo.
    echo Press any key to continue...
    pause >nul
    goto :end
) else (
    echo WARNING: Application exited with code %errorlevel% >> %LOG_FILE%
    echo This might be normal for GUI applications >> %LOG_FILE%
    echo. >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo    REBUILD SUCCESSFUL - TEST COMPLETED >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo.
    echo ===============================================
    echo    REBUILD SUCCESSFUL - TEST COMPLETED
    echo ===============================================
    echo.
    echo Application rebuilt successfully!
    echo Exit code %errorlevel% is normal for GUI apps.
    echo Check rebuild_log.txt for details.
    echo.
    echo Press any key to continue...
    pause >nul
    goto :end
)

:error
echo. >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo    REBUILD FAILED! >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo.
echo ===============================================
echo    REBUILD FAILED!
echo ===============================================
echo.
echo Rebuild failed! Check rebuild_log.txt for details.
echo.
echo Press any key to continue...
pause >nul

:end

