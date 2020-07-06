@echo off

mkdir ..\build
pushd ..\build
call shell
call cl -FC -Zi ..\src\main.cpp user32.lib gdi32.lib
del *.ilk
del *.obj
del *.pdb
popd
