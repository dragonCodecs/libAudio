@echo off
setlocal
cd %~2
xcopy "*.dll" "%~1" /I /Q /Y
endlocal
@echo on