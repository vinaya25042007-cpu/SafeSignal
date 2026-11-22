@echo off
title Launch SafeSignal Software & Dashboard
echo ========================================================
echo   Launching SafeSignal - Emergency Communication System
echo ========================================================
echo.
echo 1. Opening SafeSignal Web Dashboard in your default browser...
start "" "%~dp0SafeSignal_Dashboard.html"

echo 2. Opening Transmitter Firmware in Arduino IDE...
start "" "%~dp0SafeSignal_Transmitter\SafeSignal_Transmitter.ino"

echo 3. Opening Receiver Firmware in Arduino IDE...
start "" "%~dp0SafeSignal_Receiver\SafeSignal_Receiver.ino"

echo.
echo All software components have been opened!
echo All you have to do now is wire the hardware breadboards.
echo ========================================================
timeout /t 5
