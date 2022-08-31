@echo off
set CFLAGS=-DSLOW=1 -DINTERNAL=1 -W4 -MT -wd4100 -wd4189 -wd4201 -wd4505 -wd4838 -wd4458 -nologo -Oi -Od -fp:fast -GR- -Gm- -Z7 -EHa 

set LDLIBS=gdi32.lib user32.lib winmm.lib opengl32.lib

set LDFLAGS=-incremental:no

set OPTIMIZE=/0i /02 /fp:fast

set PROJDIR=%CD%\
set TESTDIR=%PROJDIR%tests\
set BUILDDIR=%PROJDIR%build\
set OBJDIR=%BUILDDIR%obj\

set DATETIME=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~1,1%%time:~3,2%%time:~6,2%

mkdir %OBJDIR% 2> NUL

REM Clean up	
   del %BUILDDIR%*.pdb

REM Game code
	cl %CFLAGS% /I%PROJDIR% -Fo:%OBJDIR% -Fd:%BUILDDIR% %PROJDIR%main\game.cpp /link %LDFLAGS% /DLL /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /OUT:%BUILDDIR%game.dll /PDB:%BUILDDIR%game_%DATETIME%.pdb

REM Platform code
	cl %CFLAGS% /I%PROJDIR% /Fo:%OBJDIR% /Fd:%OBJDIR% %PROJDIR%main\win32_platform.cpp /link %LDFLAGS% %LDLIBS% /OUT:%BUILDDIR%game.exe

REM Test code
	cl %CFLAGS% /I%PROJDIR% /Fo:%OBJDIR% /Fd:%OBJDIR% %TESTDIR%test.cpp /link %LDFLAGS% %LDLIBS% /OUT:%BUILDDIR%test.exe

REM Copy Assets
	xcopy /S /Q %PROJDIR%data %BUILDDIR% /Y
