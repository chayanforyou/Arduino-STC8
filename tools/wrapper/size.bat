@echo off
where python >nul 2>&1 && (
    python "%~dp0size_wrapper.py" %*
    exit /b %errorlevel%
)
where python3 >nul 2>&1 && (
    python3 "%~dp0size_wrapper.py" %*
    exit /b %errorlevel%
)
where py >nul 2>&1 && (
    py -3 "%~dp0size_wrapper.py" %*
    exit /b %errorlevel%
)
python "%~dp0size_wrapper.py" %*
exit /b %errorlevel%
