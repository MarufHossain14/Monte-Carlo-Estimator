@echo off
echo Monte Carlo π Estimator - Build and Run Script
echo ===============================================

REM Check if g++ is available
where g++ >nul 2>nul
if %errorlevel% neq 0 (
    echo Error: g++ compiler not found in PATH
    echo Please install MinGW-w64 or another C++ compiler
    echo and add it to your PATH environment variable
    pause
    exit /b 1
)

echo Building Monte Carlo π Estimator...
g++ -std=c++17 -O3 -o monte_carlo_pi.exe monte_carlo_pi.cpp

if %errorlevel% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo Running Monte Carlo π Estimator with default settings...
echo.

monte_carlo_pi.exe

echo.
echo ===============================================
echo Available options:
echo   monte_carlo_pi.exe -h              (show help)
echo   monte_carlo_pi.exe -p              (show progress)
echo   monte_carlo_pi.exe -n 100000 -p    (custom samples with progress)
echo   monte_carlo_pi.exe -s results.csv  (save to CSV)
echo   monte_carlo_pi.exe -s conv.csv -i  (save intermediate results)
echo ===============================================
echo.

pause 