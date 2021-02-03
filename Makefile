CXX = @cl
CPPFLAGS = -DSLOW=1 -DINTERNAL=1 -W4 -MT -wd4701 -wd4100 -wd4189 -nologo -Oi -Od -GR- -Gm- -Z7 -EHa
LDLIBS = gdi32.lib user32.lib winmm.lib
LDFLAGS = -incremental:no
32BITFLAGS = -opt:ref -subsystem:windows,5.1

PROJDIR := $(realpath $(CURDIR)/)/
SOURCEDIR := $(PROJDIR)src/
BUILDDIR := $(PROJDIR)build/
OBJDIR := $(BUILDDIR)obj/

DATETIME = $(shell echo %date:~-4,4%%date:~-10,2%%date:~-7,2%_%time:~1,1%%time:~3,2%%time:~6,2%)


32bit:
	@cd $(BUILDDIR) && del *.pdb

	$(CXX) $(CPPFLAGS) -Fo:$(OBJDIR) -Fd:$(BUILDDIR) $(SOURCEDIR)/game.cpp /link $(LDFLAGS) $(32BITFLAGS) /DLL /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /OUT:$(BUILDDIR)game.dll /PDB:$(BUILDDIR)game_$(DATETIME).pdb

	$(CXX) $(CPPFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/win32_platform.cpp /link $(32BITFLAGS) $(LDFLAGS) $(LDLIBS) /OUT:$(BUILDDIR)game.exe

64bit: 
	@cd $(BUILDDIR) && del *.pdb

	$(CXX) $(CPPFLAGS) -Fo:$(OBJDIR) -Fd:$(BUILDDIR) $(SOURCEDIR)game.cpp /link $(LDFLAGS) /DLL /EXPORT:GameUpdateAndRender /EXPORT:GameOutputSound /OUT:$(BUILDDIR)game.dll /PDB:$(BUILDDIR)game_$(DATETIME).pdb

	$(CXX) $(CPPFLAGS) /Fo:$(OBJDIR) /Fd:$(OBJDIR) $(SOURCEDIR)/win32_platform.cpp /link $(LDFLAGS) $(LDLIBS) /OUT:$(BUILDDIR)game.exe