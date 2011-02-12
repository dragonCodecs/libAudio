@echo off
set oldPWD=%CD%
cd %~2

xcopy "*.dll" "%~1" /I /Q /Y

cd %oldPWD%
set oldPWD=
@echo on