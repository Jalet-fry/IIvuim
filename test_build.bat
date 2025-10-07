@echo off
echo Starting build test...
echo.

set LOG_FILE=build_test.txt

echo =============================================== > %LOG_FILE%
echo    BUILD TEST LOG >> %LOG_FILE%
echo    Date: %date% %time% >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%
echo. >> %LOG_FILE%

echo 1. Test qmake >> %LOG_FILE%
echo -------------- >> %LOG_FILE%
qmake -v >> %LOG_FILE% 2>&1
echo. >> %LOG_FILE%

echo 2. Test qmake on project >> %LOG_FILE%
echo ------------------------- >> %LOG_FILE%
if exist "IIvuim.pro" (
    echo IIvuim.pro found, running qmake... >> %LOG_FILE%
    qmake IIvuim.pro >> %LOG_FILE% 2>&1
    echo qmake exit code: %errorlevel% >> %LOG_FILE%
) else (
    echo IIvuim.pro NOT FOUND >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 3. Check if Makefile was created >> %LOG_FILE%
echo --------------------------------- >> %LOG_FILE%
if exist "Makefile" (
    echo Makefile created successfully >> %LOG_FILE%
    dir Makefile >> %LOG_FILE%
) else (
    echo Makefile NOT CREATED >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 4. Test make >> %LOG_FILE%
echo ------------ >> %LOG_FILE%
if exist "Makefile" (
    echo Running make... >> %LOG_FILE%
    make >> %LOG_FILE% 2>&1
    echo make exit code: %errorlevel% >> %LOG_FILE%
) else (
    echo Cannot run make - no Makefile >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 5. Check if executable was created >> %LOG_FILE%
echo ----------------------------------- >> %LOG_FILE%
if exist "SimpleLabsMenu.exe" (
    echo SimpleLabsMenu.exe created successfully >> %LOG_FILE%
    dir SimpleLabsMenu.exe >> %LOG_FILE%
) else (
    echo SimpleLabsMenu.exe NOT CREATED >> %LOG_FILE%
)
echo. >> %LOG_FILE%

echo 6. Test simple C++ compilation >> %LOG_FILE%
echo ------------------------------- >> %LOG_FILE%
echo #include ^<iostream^> > test.cpp
echo int main() { std::cout ^<^< "test" ^<^< std::endl; return 0; } >> test.cpp
echo Testing g++ compilation... >> %LOG_FILE%
g++ -o test.exe test.cpp >> %LOG_FILE% 2>&1
echo g++ exit code: %errorlevel% >> %LOG_FILE%
if exist "test.exe" (
    echo test.exe created successfully >> %LOG_FILE%
    del test.exe
) else (
    echo test.exe NOT CREATED >> %LOG_FILE%
)
del test.cpp
echo. >> %LOG_FILE%

echo =============================================== >> %LOG_FILE%
echo    BUILD TEST COMPLETED >> %LOG_FILE%
echo =============================================== >> %LOG_FILE%

echo.
echo Build test completed!
echo Results saved to: %LOG_FILE%
echo.
pause
