@echo off
set CFLAGS=-DSLOW=1 -DINTERNAL=1 -W4 -MT -wd4701 -wd4100 -wd4189 -wd4201 -nologo -Oi -Od -GR- -Gm- -Z7 -EHa 

set LDLIBS=gdi32.lib user32.lib winmm.lib

set LDFLAGS=-incremental:no

set OPTIMIZE=/0i /02 /fp:fast

set PROJDIR=%CD%\
set SOURCEDIR=%PROJDIR%src\
set TESTDIR=%PROJDIR%test\
set BUILDDIR=%PROJDIR%build\
set OBJDIR=%BUILDDIR%obj\

set DATETIME=%date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~1,1%%time:~3,2%%time:~6,2%

REM Clean up	
   del %BUILDDIR%*.pdb

REM Platform code
	cl %CFLAGS% /Fo:%OBJDIR% /Fd:%OBJDIR% %SOURCEDIR%win32_platform.cpp /link %LDFLAGS% %LDLIBS% /OUT:%BUILDDIR%game.exe

REM Game code
	cl %CFLAGS% -Fo:%OBJDIR% -Fd:%BUILDDIR% %SOURCEDIR%game.cpp /link %LDFLAGS% /DLL /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /OUT:%BUILDDIR%game.dll /PDB:%BUILDDIR%game_%DATETIME%.pdb

REM Test code
	cl %CFLAGS% /I%SOURCEDIR% /Fo:%OBJDIR% /Fd:%OBJDIR% %TESTDIR%test.cpp /link %LDFLAGS% %LDLIBS% /OUT:%BUILDDIR%test.exe