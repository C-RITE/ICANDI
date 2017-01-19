# Microsoft Visual C++ generated build script - Do not modify

INCLUDE = $(MSVC152)\INCLUDE
LIB = $(MSVC152)\LIB
OUTDIR=.\WIN16

PROJ = WDREG16
PROGTYPE = 0
CALLER = 
ARGS = 
DLLS = 
D_RCDEFINES = -d_DEBUG
R_RCDEFINES = -dNDEBUG
ORIGIN = MSVC
ORIGIN_VER = 1.00
USEMFC = 0
CC = $(MSVC152)\bin\cl.exe
CPP = $(MSVC152)\bin\cl.exe
CXX = $(MSVC152)\bin\cl.exe
CCREATEPCHFLAG = 
CPPCREATEPCHFLAG = 
CUSEPCHFLAG = 
CPPUSEPCHFLAG = 
FIRSTC =             
FIRSTCPP = 
RC = rc
CFLAGS_D_WEXE = /nologo /G2 /W3 /vmg /vms /Zi /AM /YX /Od /D "_DEBUG" /D "WIN16" /I "$(WDSRC)/include" /I "$(WDSRC)" /I "..\..\include" /I "$(DDKROOT98)/inc/win98/inc16" /FR /GA /Fd"$(OUTDIR)/$(PROJ).PDB" /Fp"$(PROJ).PCH" /FR"$(OUTDIR)/" /Fo"$(OUTDIR)/" /I "$(WDREG16_INCLUDE)"
CFLAGS_R_WEXE = /nologo /W3 /vmg /vms /AM /YX /O1 /D "NDEBUG" /D "WIN16" /I "..\..\include" /I "$(DDKROOT98)/inc/win98/inc16" /FR /GA /Fp"$(OUTDIR)/$(PROJ).PCH" /FR"$(OUTDIR)/" /Fo"$(OUTDIR)/" /I "$(WDREG16_INCLUDE)"
LFLAGS_D_WEXE = /L:"$(WDREG16_LIB)" /NOLOGO /NOD /PACKC:61440 /STACK:32768 /ALIGN:16 /ONERROR:NOEXE /CO  
LFLAGS_R_WEXE = /L:"$(WDREG16_LIB)" /NOLOGO /NOD /PACKC:61440 /STACK:32768 /ALIGN:16 /ONERROR:NOEXE  
LIBS_D_WEXE = setupx oldnames libw mlibcew commdlg.lib olecli.lib olesvr.lib shell.lib 
LIBS_R_WEXE = setupx oldnames libw mlibcew commdlg.lib olecli.lib olesvr.lib shell.lib 
RCFLAGS = /nologo
RESFLAGS = /nologo
RUNFLAGS = 
DEFFILE = wdreg16.def
OBJS_EXT = 
LIBS_EXT = 
!if "$(DEBUG)" == "1"
CFLAGS = $(CFLAGS_D_WEXE)
LFLAGS = $(LFLAGS_D_WEXE)
LIBS = $(LIBS_D_WEXE)
MAPFILE = nul
RCDEFINES = $(D_RCDEFINES)
!else
CFLAGS = $(CFLAGS_R_WEXE)
LFLAGS = $(LFLAGS_R_WEXE)
LIBS = $(LIBS_R_WEXE)
MAPFILE = nul
RCDEFINES = $(R_RCDEFINES)
!endif
!if [if exist MSVC.BND del MSVC.BND]
!endif
SBRS = "$(OUTDIR)\COMMON16.SBR" \
                "$(OUTDIR)\UTILS.SBR" \
                "$(OUTDIR)\WDREG.SBR" \
                "$(OUTDIR)\UI.SBR" \
                "$(OUTDIR)\CSTRING.SBR" \
                "$(OUTDIR)\DRIVERWDM16.SBR" \
                "$(OUTDIR)\DRIVER98.SBR"


COMMON16_DEP = $(MSVC152)\include\tchar.h \
        common16.h \
        $(DDKROOT98)\inc\win98\setupapi.h \
        $(DDKROOT98)\inc\win98\pshpack1.h \
        $(DDKROOT98)\inc\win98\inc16\commctrl.h \
        $(DDKROOT98)\inc\win98\inc16\poppack.h \
        $(DDKROOT98)\inc\win98\cfgmgr32.h \
        $(DDKROOT98)\inc\win98\cfg.h \
        $(DDKROOT98)\inc\win98\inc16\setupx.h \
        $(DDKROOT98)\inc\win98\inc16\prsht.h


UI_DEP = $(MSVC152)\include\tchar.h \
        ..\..\include\cstring.h \
        ..\..\include\utils.h \
        ui.h


WDREG_DEP = $(DDKROOT98)\inc\win98\setupapi.h \
        $(DDKROOT98)\inc\win98\pshpack1.h \
        $(DDKROOT98)\inc\win98\inc16\commctrl.h \
        $(DDKROOT98)\inc\win98\inc16\poppack.h \
        common16.h \
        $(DDKROOT98)\inc\win98\cfgmgr32.h \
        $(DDKROOT98)\inc\win98\cfg.h \
        $(DDKROOT98)\inc\win98\inc16\setupx.h \
        $(DDKROOT98)\inc\win98\inc16\prsht.h \
        driver.h \
        ui.h \
        ..\..\include\cstring.h \
        ..\..\include\utils.h


UTILS_DEP = ..\..\include\utils.h \
        ..\..\include\cstring.h \
        $(DDKROOT98)\inc\win98\winbase.h \
        $(DDKROOT98)\inc\win98\inc16\winerror.h


CSTRING_DEP = ..\..\include\cstring.h


DRIVERWDM16_DEP = common16.h \
        $(DDKROOT98)\inc\win98\setupapi.h \
        $(DDKROOT98)\inc\win98\pshpack1.h \
        $(DDKROOT98)\inc\win98\inc16\commctrl.h \
        $(DDKROOT98)\inc\win98\inc16\poppack.h \
        $(DDKROOT98)\inc\win98\cfgmgr32.h \
        $(DDKROOT98)\inc\win98\cfg.h \
        $(DDKROOT98)\inc\win98\inc16\setupx.h \
        $(DDKROOT98)\inc\win98\inc16\prsht.h \
        ..\..\include\utils.h \
        ..\..\include\cstring.h \
        driver.h

DRIVER98_DEP = common16.h \
        $(DDKROOT98)\inc\win98\setupapi.h \
        $(DDKROOT98)\inc\win98\pshpack1.h \
        $(DDKROOT98)\inc\win98\inc16\commctrl.h \
        $(DDKROOT98)\inc\win98\inc16\poppack.h \
        $(DDKROOT98)\inc\win98\cfgmgr32.h \
        $(DDKROOT98)\inc\win98\cfg.h \
        $(DDKROOT98)\inc\win98\inc16\setupx.h \
        $(DDKROOT98)\inc\win98\inc16\prsht.h \
        ..\..\include\utils.h \
        ..\..\include\cstring.h \
        driver.h

all:  "$(OUTDIR)\$(PROJ).EXE" "$(OUTDIR)\$(PROJ).BSC"

"$(OUTDIR)":
    if not exist "$(OUTDIR)" mkdir "$(OUTDIR)"

clean: 
    if exist "$(OUTDIR)" del /S /Q "$(OUTDIR)" >NUL
    if exist "$(OUTDIR)" rd "$(OUTDIR)" 

"$(OUTDIR)\COMMON16.OBJ": "$(OUTDIR)"   COMMON16.CPP $(COMMON16_DEP)
        $(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c COMMON16.CPP

"$(OUTDIR)\UI.OBJ": "$(OUTDIR)" UI.CPP $(UI_DEP)
        $(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c UI.CPP

"$(OUTDIR)\WDREG.OBJ": "$(OUTDIR)"      WDREG.CPP $(WDREG_DEP)
        $(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c WDREG.CPP

"$(OUTDIR)\UTILS.OBJ": "$(OUTDIR)"      ..\..\SRC\WDAPI\UTILS.C $(UTILS_DEP)
        $(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\..\SRC\WDAPI\UTILS.C

"$(OUTDIR)\CSTRING.OBJ": "$(OUTDIR)"    ..\..\SRC\WDAPI\CSTRING.CPP $(CSTRING_DEP)
        $(CPP) $(CFLAGS) $(CPPUSEPCHFLAG) /c ..\..\SRC\WDAPI\CSTRING.CPP

"$(OUTDIR)\DRIVERWDM16.OBJ": "$(OUTDIR)"        DRIVERWDM16.CPP $(DRIVERWDM16_DEP)
        $(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c DRIVERWDM16.CPP

"$(OUTDIR)\DRIVER98.OBJ": "$(OUTDIR)"   DRIVER98.CPP $(DRIVER98_DEP)
        $(CPP) $(CFLAGS) $(CPPCREATEPCHFLAG) /c DRIVER98.CPP


"$(OUTDIR)\$(PROJ).EXE" :       "$(OUTDIR)" "$(OUTDIR)\COMMON16.OBJ" "$(OUTDIR)\UI.OBJ" "$(OUTDIR)\WDREG.OBJ" "$(OUTDIR)\UTILS.OBJ" "$(OUTDIR)\CSTRING.OBJ" "$(OUTDIR)\DRIVERWDM16.OBJ" "$(OUTDIR)\DRIVER98.OBJ" $(OBJ_EXT) $(DEFFILE)
        echo >NUL @<<$(PROJ).CRF
"$(OUTDIR)\COMMON16.OBJ" +
"$(OUTDIR)\UI.OBJ" +
"$(OUTDIR)\WDREG.OBJ" +
"$(OUTDIR)\UTILS.OBJ" +
"$(OUTDIR)\CSTRING.OBJ" +
"$(OUTDIR)\DRIVERWDM16.OBJ" +
"$(OUTDIR)\DRIVER98.OBJ" +
$(OBJS_EXT)
"$(OUTDIR)\$(PROJ).EXE"
$(MAPFILE)
$(MSVC152)\lib\+
$(MSVC152)\mfc\lib\+
$(DDKROOT98)\lib\win98\+
$(LIBS)
$(DEFFILE);
<<
        $(MSVC152)\BIN\link $(LFLAGS) @$(PROJ).CRF


run: $(PROJ).EXE
        $(PROJ) $(RUNFLAGS)


"$(OUTDIR)\$(PROJ).BSC": $(SBRS)
        $(MSVC152)\BIN\bscmake @<<
/o$@ $(SBRS)
<<
