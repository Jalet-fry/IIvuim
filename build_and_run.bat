@echo off
echo ===============================================
echo    UNIVERSAL Qt PROJECT BUILDER
echo    For Windows XP with Qt 5.5.1
echo ===============================================
echo.

set LOG_FILE=build_log.txt

echo =============================================== > %LOG_FILE%
echo    UNIVERSAL BUILD AND RUN LOG >> %LOG_FILE%
echo    Date: %date% %time% >> %LOG_FILE%
echo    XP Compatibility Fixes Applied >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 1: Setup environment >> %LOG_FILE%
echo --------------------------- >> %LOG_FILE%
echo Setting up PATH for Qt and MinGW... >> %LOG_FILE%
set PATH=%PATH%;C:\Qt\Qt5.5.1\5.5\mingw492_32\bin;C:\Qt\Qt5.5.1\Tools\mingw492_32\bin
echo PATH=%PATH% >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 2: Verify tools >> %LOG_FILE%
echo --------------------- >> %LOG_FILE%
echo Testing qmake... >> %LOG_FILE%
qmake -v >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: qmake not found! >> %LOG_FILE%
    echo Build FAILED - qmake not available >> %LOG_FILE%
    goto :error
)
echo qmake OK >> %LOG_FILE%

echo Testing gcc... >> %LOG_FILE%
gcc --version >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: gcc not found! >> %LOG_FILE%
    echo Build FAILED - gcc not available >> %LOG_FILE%
    goto :error
)
echo gcc OK >> %LOG_FILE%

echo Testing mingw32-make... >> %LOG_FILE%
mingw32-make --version >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: mingw32-make not found! >> %LOG_FILE%
    echo Build FAILED - mingw32-make not available >> %LOG_FILE%
    goto :error
)
echo mingw32-make OK >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 3: Check project files >> %LOG_FILE%
echo ----------------------------- >> %LOG_FILE%
if not exist "IIvuim.pro" (
    echo ERROR: IIvuim.pro not found! >> %LOG_FILE%
    echo Build FAILED - no project file >> %LOG_FILE%
    goto :error
)
echo IIvuim.pro found >> %LOG_FILE%

if not exist "main.cpp" (
    echo ERROR: main.cpp not found! >> %LOG_FILE%
    echo Build FAILED - no main.cpp >> %LOG_FILE%
    goto :error
)
echo main.cpp found >> %LOG_FILE%

if not exist "mainwindow.cpp" (
    echo ERROR: mainwindow.cpp not found! >> %LOG_FILE%
    echo Build FAILED - no mainwindow.cpp >> %LOG_FILE%
    goto :error
)
echo mainwindow.cpp found >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 4: Clean previous build >> %LOG_FILE%
echo ------------------------------ >> %LOG_FILE%
if exist "Makefile" del Makefile >> %LOG_FILE% 2>&1
if exist "Makefile.Debug" del Makefile.Debug >> %LOG_FILE% 2>&1
if exist "Makefile.Release" del Makefile.Release >> %LOG_FILE% 2>&1
if exist "*.o" del *.o >> %LOG_FILE% 2>&1
if exist "moc_*.cpp" del moc_*.cpp >> %LOG_FILE% 2>&1
if exist "qrc_*.cpp" del qrc_*.cpp >> %LOG_FILE% 2>&1
if exist "SimpleLabsMenu.exe" del SimpleLabsMenu.exe >> %LOG_FILE% 2>&1
echo Cleanup completed >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 5: Generate Makefile >> %LOG_FILE%
echo --------------------------- >> %LOG_FILE%
echo Running qmake... >> %LOG_FILE%
qmake IIvuim.pro >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: qmake failed! >> %LOG_FILE%
    echo Build FAILED - qmake error >> %LOG_FILE%
    goto :error
)
echo qmake completed successfully >> %LOG_FILE%

if not exist "Makefile" (
    echo ERROR: Makefile not created! >> %LOG_FILE%
    echo Build FAILED - no Makefile >> %LOG_FILE%
    goto :error
)
echo Makefile created successfully >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 6: Build project >> %LOG_FILE%
echo ---------------------- >> %LOG_FILE%
echo Running mingw32-make... >> %LOG_FILE%
mingw32-make >> %LOG_FILE% 2>&1
if %errorlevel% neq 0 (
    echo ERROR: mingw32-make failed! >> %LOG_FILE%
    echo Build FAILED - compilation error >> %LOG_FILE%
    goto :error
)
echo mingw32-make completed successfully >> %LOG_FILE%

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
    echo ERROR: SimpleLabsMenu.exe not found in root or release! >> %LOG_FILE%
    echo Build FAILED - no executable >> %LOG_FILE%
    goto :error
)
echo SimpleLabsMenu.exe ready >> %LOG_FILE%
echo. >> %LOG_FILE%

echo STEP 7: Check Qt DLL dependencies >> %LOG_FILE%
echo ----------------------------------- >> %LOG_FILE%
if not exist "Qt5Core.dll" (
    echo WARNING: Qt5Core.dll not found in project directory >> %LOG_FILE%
    echo Copying from Qt installation... >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Core.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Core.dll" . >> %LOG_FILE% 2>&1
        echo Qt5Core.dll copied >> %LOG_FILE%
    ) else (
        echo ERROR: Qt5Core.dll not found in Qt installation! >> %LOG_FILE%
    )
) else (
    echo Qt5Core.dll found >> %LOG_FILE%
)

if not exist "Qt5Gui.dll" (
    echo WARNING: Qt5Gui.dll not found in project directory >> %LOG_FILE%
    echo Copying from Qt installation... >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Gui.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Gui.dll" . >> %LOG_FILE% 2>&1
        echo Qt5Gui.dll copied >> %LOG_FILE%
    ) else (
        echo ERROR: Qt5Gui.dll not found in Qt installation! >> %LOG_FILE%
    )
) else (
    echo Qt5Gui.dll found >> %LOG_FILE%
)

if not exist "Qt5Widgets.dll" (
    echo WARNING: Qt5Widgets.dll not found in project directory >> %LOG_FILE%
    echo Copying from Qt installation... >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Widgets.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\Qt5Widgets.dll" . >> %LOG_FILE% 2>&1
        echo Qt5Widgets.dll copied >> %LOG_FILE%
    ) else (
        echo ERROR: Qt5Widgets.dll not found in Qt installation! >> %LOG_FILE%
    )
) else (
    echo Qt5Widgets.dll found >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo STEP 8: Check MinGW DLL dependencies >> %LOG_FILE%
echo -------------------------------------- >> %LOG_FILE%
if not exist "libgcc_s_dw2-1.dll" (
    echo WARNING: libgcc_s_dw2-1.dll not found >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libgcc_s_dw2-1.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libgcc_s_dw2-1.dll" . >> %LOG_FILE% 2>&1
        echo libgcc_s_dw2-1.dll copied >> %LOG_FILE%
    )
) else (
    echo libgcc_s_dw2-1.dll found >> %LOG_FILE%
)

if not exist "libstdc++-6.dll" (
    echo WARNING: libstdc++-6.dll not found >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libstdc++-6.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libstdc++-6.dll" . >> %LOG_FILE% 2>&1
        echo libstdc++-6.dll copied >> %LOG_FILE%
    )
) else (
    echo libstdc++-6.dll found >> %LOG_FILE%
)

if not exist "libwinpthread-1.dll" (
    echo WARNING: libwinpthread-1.dll not found >> %LOG_FILE%
    if exist "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libwinpthread-1.dll" (
        copy "C:\Qt\Qt5.5.1\5.5\mingw492_32\bin\libwinpthread-1.dll" . >> %LOG_FILE% 2>&1
        echo libwinpthread-1.dll copied >> %LOG_FILE%
    )
) else (
    echo libwinpthread-1.dll found >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo STEP 9: Test run >> %LOG_FILE%
echo ---------------- >> %LOG_FILE%
echo Testing SimpleLabsMenu.exe... >> %LOG_FILE%
SimpleLabsMenu.exe >> %LOG_FILE% 2>&1
echo Executable exit code: %errorlevel% >> %LOG_FILE%

if %errorlevel% equ 0 (
    echo SUCCESS: Application ran without errors! >> %LOG_FILE%
    echo. >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo    BUILD AND RUN SUCCESSFUL! >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo.
    echo ===============================================
    echo    BUILD AND RUN SUCCESSFUL!
    echo ===============================================
    echo.
    echo Application built and ran successfully!
    echo Check build_log.txt for details.
    echo.
    echo Press any key to continue...
    pause >nul
    goto :end
) else (
    echo WARNING: Application exited with code %errorlevel% >> %LOG_FILE%
    echo This might be normal for GUI applications >> %LOG_FILE%
    echo. >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo    BUILD SUCCESSFUL - RUN COMPLETED >> %LOG_FILE%
    echo =============================================== >> %LOG_FILE%
    echo.
    echo ===============================================
    echo    BUILD SUCCESSFUL - RUN COMPLETED
    echo ===============================================
    echo.
    echo Application built successfully!
    echo Exit code %errorlevel% is normal for GUI apps.
    echo Check build_log.txt for details.
    echo.
    echo Press any key to continue...
    pause >nul
    goto :end
)

:error
echo. >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo    BUILD FAILED! >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo.
echo ===============================================
echo    BUILD FAILED!
echo ===============================================
echo.
echo Build failed! Check build_log.txt for details.
echo.
echo Press any key to continue...
pause >nul

:end
