# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=snom32ms - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to snom32ms - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "snom32ms - Win32 Release" && "$(CFG)" !=\
 "snom32ms - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "snom32ms.mak" CFG="snom32ms - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "snom32ms - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "snom32ms - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 
################################################################################
# Begin Project
# PROP Target_Last_Scanned "snom32ms - Win32 Debug"
CPP=cl.exe
RSC=rc.exe
MTL=mktyplib.exe

!IF  "$(CFG)" == "snom32ms - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
OUTDIR=.\Release
INTDIR=.\Release

ALL : "$(OUTDIR)\snom32ms.exe"

CLEAN : 
	-@erase ".\Release\snom32ms.exe"
	-@erase ".\Release\snom-fit.obj"
	-@erase ".\Release\snomputz.obj"
	-@erase ".\Release\Dsp-mes.obj"
	-@erase ".\Release\avi_utils.obj"
	-@erase ".\Release\snom-prg.obj"
	-@erase ".\Release\snom-bmp.obj"
	-@erase ".\Release\snom-mat.obj"
	-@erase ".\Release\snom-dlg.obj"
	-@erase ".\Release\snom-avi.obj"
	-@erase ".\Release\snom-wrk.obj"
	-@erase ".\Release\snom-dsp.obj"
	-@erase ".\Release\snom-dat.obj"
	-@erase ".\Release\snom-mem.obj"
	-@erase ".\Release\Filebox.obj"
	-@erase ".\Release\snom-pac.obj"
	-@erase ".\Release\snomputz.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Oi /Os /Op /Oy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /Oi /Os /Op /Oy /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/snom32ms.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/snomputz.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/snom32ms.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib vfw32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /force
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/snom32ms.pdb" /machine:I386 /out:"$(OUTDIR)/snom32ms.exe" 
LINK32_OBJS= \
	"$(INTDIR)/snom-fit.obj" \
	"$(INTDIR)/snomputz.obj" \
	"$(INTDIR)/Dsp-mes.obj" \
	"$(INTDIR)/avi_utils.obj" \
	"$(INTDIR)/snom-prg.obj" \
	"$(INTDIR)/snom-bmp.obj" \
	"$(INTDIR)/snom-mat.obj" \
	"$(INTDIR)/snom-dlg.obj" \
	"$(INTDIR)/snom-avi.obj" \
	"$(INTDIR)/snom-wrk.obj" \
	"$(INTDIR)/snom-dsp.obj" \
	"$(INTDIR)/snom-dat.obj" \
	"$(INTDIR)/snom-mem.obj" \
	"$(INTDIR)/Filebox.obj" \
	"$(INTDIR)/snom-pac.obj" \
	"$(INTDIR)/snomputz.res"

"$(OUTDIR)\snom32ms.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
OUTDIR=.\Debug
INTDIR=.\Debug

ALL : "$(OUTDIR)\snom32ms.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\snom32ms.exe"
	-@erase ".\Debug\snom-pac.obj"
	-@erase ".\Debug\snom-fit.obj"
	-@erase ".\Debug\snom-mat.obj"
	-@erase ".\Debug\snomputz.obj"
	-@erase ".\Debug\avi_utils.obj"
	-@erase ".\Debug\snom-wrk.obj"
	-@erase ".\Debug\Filebox.obj"
	-@erase ".\Debug\snom-bmp.obj"
	-@erase ".\Debug\snom-mem.obj"
	-@erase ".\Debug\Dsp-mes.obj"
	-@erase ".\Debug\snom-dlg.obj"
	-@erase ".\Debug\snom-avi.obj"
	-@erase ".\Debug\snom-dsp.obj"
	-@erase ".\Debug\snom-dat.obj"
	-@erase ".\Debug\snom-prg.obj"
	-@erase ".\Debug\snomputz.res"
	-@erase ".\Debug\snom32ms.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/snom32ms.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/snomputz.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/snom32ms.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /force
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/snom32ms.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/snom32ms.exe" 
LINK32_OBJS= \
	"$(INTDIR)/snom-pac.obj" \
	"$(INTDIR)/snom-fit.obj" \
	"$(INTDIR)/snom-mat.obj" \
	"$(INTDIR)/snomputz.obj" \
	"$(INTDIR)/avi_utils.obj" \
	"$(INTDIR)/snom-wrk.obj" \
	"$(INTDIR)/Filebox.obj" \
	"$(INTDIR)/snom-bmp.obj" \
	"$(INTDIR)/snom-mem.obj" \
	"$(INTDIR)/Dsp-mes.obj" \
	"$(INTDIR)/snom-dlg.obj" \
	"$(INTDIR)/snom-avi.obj" \
	"$(INTDIR)/snom-dsp.obj" \
	"$(INTDIR)/snom-dat.obj" \
	"$(INTDIR)/snom-prg.obj" \
	"$(INTDIR)/snomputz.res"

"$(OUTDIR)\snom32ms.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_OBJS)}.obj:
   $(CPP) $(CPP_PROJ) $<  

.c{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cpp{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

.cxx{$(CPP_SBRS)}.sbr:
   $(CPP) $(CPP_PROJ) $<  

################################################################################
# Begin Target

# Name "snom32ms - Win32 Release"
# Name "snom32ms - Win32 Debug"

!IF  "$(CFG)" == "snom32ms - Win32 Release"

!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

!ENDIF 

################################################################################
# Begin Source File

SOURCE=".\snom-bmp.c"
DEP_CPP_SNOM_=\
	".\Myportab.h"\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\snom-dlg.h"\
	".\snom-dsp.h"\
	".\snom-dat.h"\
	".\snom-wrk.h"\
	".\snom-mat.h"\
	".\Filebox.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	

"$(INTDIR)\snom-bmp.obj" : $(SOURCE) $(DEP_CPP_SNOM_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Filebox.c
DEP_CPP_FILEB=\
	".\Filebox.h"\
	".\snomputz.h"\
	".\snomlang.h"\
	

"$(INTDIR)\Filebox.obj" : $(SOURCE) $(DEP_CPP_FILEB) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-dat.c"
DEP_CPP_SNOM_D=\
	".\Myportab.h"\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-pac.h"\
	".\snom-dat.h"\
	".\snom-mem.h"\
	".\snom-mat.h"\
	".\snom-wrk.h"\
	".\snom-win.h"\
	".\snom-dlg.h"\
	".\Hdf-file.h"\
	".\Hitachi.h"\
	".\WSxM.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Hntdefs.h"\
	".\Htags.h"\
	

"$(INTDIR)\snom-dat.obj" : $(SOURCE) $(DEP_CPP_SNOM_D) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-dlg.c"
DEP_CPP_SNOM_DL=\
	".\Myportab.h"\
	".\Filebox.h"\
	".\snomputz.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-dlg.h"\
	".\snom-dat.h"\
	".\snom-dsp.h"\
	".\snom-mem.h"\
	".\snom-wrk.h"\
	".\snom-win.h"\
	".\snom-mat.h"\
	".\snom-def.h"\
	".\snom-typ.h"\
	".\Psi-hdf.h"\
	

"$(INTDIR)\snom-dlg.obj" : $(SOURCE) $(DEP_CPP_SNOM_DL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-dsp.c"
DEP_CPP_SNOM_DS=\
	".\Myportab.h"\
	".\snom-typ.h"\
	".\snomputz.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-dsp.h"\
	".\snom-wrk.h"\
	".\snom-mem.h"\
	".\snom-mat.h"\
	".\snom-win.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	

"$(INTDIR)\snom-dsp.obj" : $(SOURCE) $(DEP_CPP_SNOM_DS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-fit.c"

!IF  "$(CFG)" == "snom32ms - Win32 Release"

DEP_CPP_SNOM_F=\
	".\Nrutil.h"\
	".\snom-typ.h"\
	".\snom-win.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-fit.obj" : $(SOURCE) $(DEP_CPP_SNOM_F) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

DEP_CPP_SNOM_F=\
	".\Nrutil.h"\
	".\Myportab.h"\
	".\snom-typ.h"\
	".\snom-win.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	

"$(INTDIR)\snom-fit.obj" : $(SOURCE) $(DEP_CPP_SNOM_F) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-mat.c"
DEP_CPP_SNOM_M=\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-fit.h"\
	".\snom-wrk.h"\
	".\snom-mem.h"\
	".\snom-win.h"\
	".\snom-mat.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-mat.obj" : $(SOURCE) $(DEP_CPP_SNOM_M) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-mem.c"
DEP_CPP_SNOM_ME=\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-mem.obj" : $(SOURCE) $(DEP_CPP_SNOM_ME) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-pac.c"
DEP_CPP_SNOM_P=\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-mem.h"\
	".\snom-win.h"\
	".\snom-pac.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-pac.obj" : $(SOURCE) $(DEP_CPP_SNOM_P) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\snomputz.c
DEP_CPP_SNOMP=\
	".\Myportab.h"\
	".\snomputz.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\Filebox.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\snom-dat.h"\
	".\snom-dlg.h"\
	".\snom-dsp.h"\
	".\snom-avi.h"\
	".\Dsp-mes.h"\
	".\snom-prg.h"\
	".\snom-def.h"\
	".\snom-typ.h"\
	".\Psi-hdf.h"\
	

"$(INTDIR)\snomputz.obj" : $(SOURCE) $(DEP_CPP_SNOMP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-wrk.c"
DEP_CPP_SNOM_W=\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-mem.h"\
	".\snom-win.h"\
	".\snom-wrk.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-wrk.obj" : $(SOURCE) $(DEP_CPP_SNOM_W) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Dsp-mes.c"

!IF  "$(CFG)" == "snom32ms - Win32 Release"

DEP_CPP_DSP_M=\
	".\Dsp\Scantyp.h"\
	".\Pc32dll\Target.h"\
	".\snomputz.h"\
	".\Myportab.h"\
	".\snom-typ.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-dat.h"\
	".\snom-dsp.h"\
	".\snom-mat.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\Pc32dll\Cardinfo.h"\
	".\Pc32dll\Alias.h"\
	".\Pc32dll\Ii_iostr.h"\
	".\Pc32dll\Mailbox.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	

"$(INTDIR)\Dsp-mes.obj" : $(SOURCE) $(DEP_CPP_DSP_M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

DEP_CPP_DSP_M=\
	".\Dsp\Scantyp.h"\
	".\Pc32dll\Target.h"\
	".\snomputz.h"\
	".\Myportab.h"\
	".\snom-typ.h"\
	".\snom-var.h"\
	".\snomlang.h"\
	".\snom-dat.h"\
	".\snom-dsp.h"\
	".\snom-mat.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	

"$(INTDIR)\Dsp-mes.obj" : $(SOURCE) $(DEP_CPP_DSP_M) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-prg.c"
DEP_CPP_SNOM_PR=\
	".\Filebox.h"\
	".\snomputz.h"\
	".\snom-typ.h"\
	".\snom-win.h"\
	".\snom-mem.h"\
	".\snom-dlg.h"\
	".\snom-dsp.h"\
	".\snom-dat.h"\
	".\snom-wrk.h"\
	".\snom-mat.h"\
	".\snom-prg.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-prg.obj" : $(SOURCE) $(DEP_CPP_SNOM_PR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\snomputz.rc
DEP_RSC_SNOMPU=\
	".\snomputz.ico"\
	".\snombild.ico"\
	".\toolbar.bmp"\
	".\scanbar.bmp"\
	".\3p.bmp"\
	".\snomputz.h"\
	".\afxres.h"\
	".\winres.h"\
	

"$(INTDIR)\snomputz.res" : $(SOURCE) $(DEP_RSC_SNOMPU) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-avi.c"
DEP_CPP_SNOM_A=\
	".\Myportab.h"\
	".\snom-typ.h"\
	".\snom-var.h"\
	".\snom-mem.h"\
	".\snomlang.h"\
	".\snom-dlg.h"\
	".\snom-dsp.h"\
	".\avi_utils.h"\
	".\Psi-hdf.h"\
	".\snom-def.h"\
	".\snomputz.h"\
	

"$(INTDIR)\snom-avi.obj" : $(SOURCE) $(DEP_CPP_SNOM_A) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\avi_utils.cpp
DEP_CPP_AVI_U=\
	".\avi_utils.h"\
	

"$(INTDIR)\avi_utils.obj" : $(SOURCE) $(DEP_CPP_AVI_U) "$(INTDIR)"


# End Source File
# End Target
# End Project
################################################################################
