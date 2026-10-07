@echo off
rem packihx wrapper for Windows
rem %~1 strips the quotes Arduino adds, so paths with spaces work

"%~1" "%~2" > "%~3" 2>nul

exit /b %errorlevel%
