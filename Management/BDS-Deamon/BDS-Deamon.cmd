@echo off
set RESTART_TIMEOUT=5

:Restart
bedrock_server_mod.exe

REM stop
if %errorlevel% EQU 0 goto End
REM Ctrl+C
if %errorlevel% EQU -1073740972 goto End

echo.
echo ==============================
echo 服务器发生崩溃，错误码：%errorlevel%
echo 等待%RESTART_TIMEOUT%秒后，服务器将自动重启 
echo ==============================
echo.
echo.
timeout /t %RESTART_TIMEOUT% >nul
goto Restart
:End
exit