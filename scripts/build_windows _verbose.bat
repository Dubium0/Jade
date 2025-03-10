@echo off
cd ..
vendor\premake\premake5.exe --verbose --file=build.lua vs2022
pause
