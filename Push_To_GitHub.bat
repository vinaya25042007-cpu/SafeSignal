@echo off
title Push SafeSignal to GitHub
echo ========================================================
echo   Pushing SafeSignal to GitHub (vinaya25042007-cpu)
echo ========================================================
echo.
cd /d "%~dp0"
git push -u origin main
echo.
if %ERRORLEVEL% EQU 0 (
    echo [SUCCESS] SafeSignal has been successfully pushed to:
    echo https://github.com/vinaya25042007-cpu/SafeSignal
) else (
    echo [NOTE] If you got 'Repository not found', please make sure
    echo you created an empty repository named 'SafeSignal' at:
    echo https://github.com/new
)
echo ========================================================
pause
