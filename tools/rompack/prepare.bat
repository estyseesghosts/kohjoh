@echo off
rem Drag your .nds onto this file, or double-click it with the .nds in the same folder.
cd /d "%~dp0"
where py >nul 2>nul && (py -3 prepare.py %*) || (python prepare.py %*)
if errorlevel 1 echo. & echo If Python was not found, install it from https://www.python.org/downloads/
pause
