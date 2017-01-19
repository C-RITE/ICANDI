DRV_NAME  = kp_pci

!ifndef BASEDIR
!ERROR set BASEDIR enviroment variable to the NTDDK path. For example: SET BASEDIR=C:\NTDDK
!endif
BINDIR     = WINNT.i386
DRV_NM     = $(DRV_NAME).sys
C_FLAGS  = -nologo -DWINVER=0x030A -D_X86_=1 -Di386=1 -DWINNT  -D__KERNEL__ -DSTD_CALL -DCONDITION_HANDLING=1 -DWIN32_LEAN_AND_MEAN=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DNT_UP=1 -Dtry=__try -Dleave=__leave -Dexcept=__except -Dfinally=__finally -D_CRTAPI1=__cdecl -D_CRTAPI2=__cdecl  -Ditoa=_itoa  -Dstrcmpi=_strcmpi  -Dstricmp=_stricmp  -Dwcsicmp=_wcsicmp  -Dwcsnicmp=_wcsnicmp  -DDBG=0 -DDEVL=1 -DFPO=1 -D_IDWBUILD /c /Zp8 /Gy /W3 /Gz /Oxs /GF /Oy -DWD_DRIVER_NAME_CHANGE
INCDIRS    = -I. -I$(BASEDIR)/inc/crt \
             -I../../.. -I../../../include -I$(BASEDIR)/inc/api
LIBDIR     = ../../../lib

OBJECTS= $(BINDIR)/kp_pci.obj $(BINDIR)/pci_lib.obj

all: direxist $(BINDIR)/$(DRV_NM)
        $(SIGN_REAL) $(BINDIR)/$(DRV_NM)

direxist:
        if not exist $(BINDIR)\nul    md $(BINDIR)

$(BINDIR)/kp_pci.obj: kp_pci.c
        $(CC) $(C_FLAGS) $(INCDIRS) -Fo$@ %s

$(BINDIR)/pci_lib.obj: ../pci_lib.c
        $(CC) $(C_FLAGS) $(INCDIRS) -Fo$@ %s

$(BINDIR)/kp_pci.res: kp_pci.rc
        $(RC) -l 409  -r -fo $@ -D_X86_=1 -Di386=1  -DSTD_CALL -DCONDITION_HANDLING=1 -DWIN32_LEAN_AND_MEAN=1 -DNT_UP=1  -DNT_INST=0 -DWIN32=100 -D_NT1X_=100 -DNT_UP=1 -Dtry=__try -Dleave=__leave -Dexcept=__except -Dfinally=__finally -D_CRTAPI1=__cdecl -D_CRTAPI2=__cdecl  -Ditoa=_itoa  -Dstrcmpi=_strcmpi  -Dstricmp=_stricmp  -Dwcsicmp=_wcsicmp  -Dwcsnicmp=_wcsnicmp  -DDBG=0 -DDEVL=1 -DFPO=1 -D_IDWBUILD $(INCDIRS) %s

$(BINDIR)/$(DRV_NM): $(OBJECTS) $(BINDIR)/kp_pci.res
        echo >NUL @<<$(DRV_NAME).crf
-MERGE:_PAGE=PAGE 
-MERGE:_TEXT=.text 
-SECTION:INIT,d 
-OPT:REF 
-RELEASE
-INCREMENTAL:NO
-FULLBUILD 
-FORCE:MULTIPLE 
-IGNORE:4001 -IGNORE:4037 -IGNORE:4039 -IGNORE:4065 -IGNORE:4070 -IGNORE:4078 -IGNORE:4087 -IGNORE:4089 
-debug:FULL
-version:6.00 
-osversion:3.51 
-MERGE:.rdata=.text 
-align:0x20 
-subsystem:native,4.00 
-machine:i386 
-base:0x10000 
-entry:DriverEntry@8
-driver
$(LIBDIR)/kp_nt$(WD_VERSION).lib
$(LIBDIR)/kp_wdapi$(WD_VERSION).lib
$(BASEDIR)/lib/wlh/i386/ntoskrnl.lib
$(BASEDIR)/lib/wlh/i386/hal.lib
$(BASEDIR)/lib/crt/i386/libcmtd.lib
$(BASEDIR)/lib/wlh/i386/kernel32.lib
$(BINDIR)/kp_pci.obj
$(BINDIR)/pci_lib.obj
$(BINDIR)/kp_pci.res

-out:$@ 
<<
        link @$(DRV_NAME).crf

clean:
        -@del /Q $(BINDIR)\*
