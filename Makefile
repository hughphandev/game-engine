CC = cl
CFLAGS = -DSLOW=1 -DINTERNAL=1 -W4 -MT -wd4701 -wd4100 -wd4189 -nologo -Oi -Od -GR- -Gm- -Z7 -EHa
LFLAGS = gdi32.lib user32.lib winmm.lib
32BITFLAGS = -opt:ref -subsystem:windows,5.1

PROJDIR := $(realpath $(CURDIR)/)
SOURCEDIR := $(PROJDIR)/src/
BUILDDIR := $(PROJDIR)/build/
OBJDIR := $(BUILDDIR)/obj/

32bit: 
	$(CC) $(CFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/game.cpp /link $(32BITFLAGS) /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /DLL /OUT:$(BUILDDIR)game.dll 

	$(CC) $(CFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/win32_platform.cpp /link $(32BITFLAGS) $(LFLAGS) /OUT:$(BUILDDIR)game.exe

64bit: 
	$(CC) $(CFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/game.cpp /link /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /DLL /OUT:$(BUILDDIR)game.dll 

	$(CC) $(CFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/win32_platform.cpp /link $(LFLAGS) /OUT:$(BUILDDIR)game.exe