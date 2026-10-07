@echo off
rem Run size_wrapper.py with the first Python 3 found. Each launch is outside a
rem ( ) block so %errorlevel% is read after the command, not before it.

where python >nul 2>&1 || goto try_python3
python "%~dp0size_wrapper.py" %*
exit /b %errorlevel%

:try_python3
where python3 >nul 2>&1 || goto try_py
python3 "%~dp0size_wrapper.py" %*
exit /b %errorlevel%

:try_py
where py >nul 2>&1 || goto no_python
py -3 "%~dp0size_wrapper.py" %*
exit /b %errorlevel%

:no_python
echo size: Python 3 is required but was not found in PATH 1>&2
exit /b 1
