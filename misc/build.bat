@echo off

mkdir ..\..\build
pushd ..\..\build
call shell
call cl -FC -Zi ..\win32_engine\src\main.cpp user32.lib gdi32.lib 
popd
