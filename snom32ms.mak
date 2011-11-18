# Microsoft Developer Studio Generated NMAKE File, Format Version 40001
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

!IF "$(CFG)" == ""
CFG=Snom32ms - Win32 Debug
!MESSAGE No configuration specified.  Defaulting to Snom32ms - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "snom32ms - Win32 Release" && "$(CFG)" !=\
 "snom32ms - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE on this makefile
!MESSAGE by defining the macro CFG on the command line.  For example:
!MESSAGE 
!MESSAGE NMAKE /f "Snom32ms.mak" CFG="Snom32ms - Win32 Debug"
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

ALL : "$(OUTDIR)\Snom32ms.exe"

CLEAN : 
	-@erase ".\Release\Snom32ms.exe"
	-@erase ".\Release\Snom-fit.obj"
	-@erase ".\Release\Snomputz.obj"
	-@erase ".\Release\Dsp-mes.obj"
	-@erase ".\Release\avi_utils.obj"
	-@erase ".\Release\snom-prg.obj"
	-@erase ".\Release\Snom-bmp.obj"
	-@erase ".\Release\Snom-mat.obj"
	-@erase ".\Release\Snom-dlg.obj"
	-@erase ".\Release\Snom-avi.obj"
	-@erase ".\Release\Snom-wrk.obj"
	-@erase ".\Release\Snom-dsp.obj"
	-@erase ".\Release\Snom-dat.obj"
	-@erase ".\Release\Snom-mem.obj"
	-@erase ".\Release\Filebox.obj"
	-@erase ".\Release\Snom-pac.obj"
	-@erase ".\Release\Snomputz.res"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /GX /Oi /Os /Op /Oy /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /ML /W3 /GX /Oi /Os /Op /Oy /D "WIN32" /D "NDEBUG" /D\
 "_WINDOWS" /Fp"$(INTDIR)/Snom32ms.pch" /YX /Fo"$(INTDIR)/" /c 
CPP_OBJS=.\Release/
CPP_SBRS=
# ADD BASE MTL /nologo /D "NDEBUG" /win32
# ADD MTL /nologo /D "NDEBUG" /win32
MTL_PROJ=/nologo /D "NDEBUG" /win32 
# ADD BASE RSC /l 0x407 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Snomputz.res" /d "NDEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Snom32ms.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib vfw32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /force
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Snom32ms.pdb" /machine:I386 /out:"$(OUTDIR)/Snom32ms.exe" 
LINK32_OBJS= \
	"$(INTDIR)/Snom-fit.obj" \
	"$(INTDIR)/Snomputz.obj" \
	"$(INTDIR)/Dsp-mes.obj" \
	"$(INTDIR)/avi_utils.obj" \
	"$(INTDIR)/snom-prg.obj" \
	"$(INTDIR)/Snom-bmp.obj" \
	"$(INTDIR)/Snom-mat.obj" \
	"$(INTDIR)/Snom-dlg.obj" \
	"$(INTDIR)/Snom-avi.obj" \
	"$(INTDIR)/Snom-wrk.obj" \
	"$(INTDIR)/Snom-dsp.obj" \
	"$(INTDIR)/Snom-dat.obj" \
	"$(INTDIR)/Snom-mem.obj" \
	"$(INTDIR)/Filebox.obj" \
	"$(INTDIR)/Snom-pac.obj" \
	"$(INTDIR)/Snomputz.res"

"$(OUTDIR)\Snom32ms.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

ALL : "$(OUTDIR)\Snom32ms.exe"

CLEAN : 
	-@erase ".\Debug\vc40.pdb"
	-@erase ".\Debug\vc40.idb"
	-@erase ".\Debug\Snom32ms.exe"
	-@erase ".\Debug\Snom-pac.obj"
	-@erase ".\Debug\Snom-fit.obj"
	-@erase ".\Debug\Snom-mat.obj"
	-@erase ".\Debug\Snomputz.obj"
	-@erase ".\Debug\avi_utils.obj"
	-@erase ".\Debug\Snom-wrk.obj"
	-@erase ".\Debug\Filebox.obj"
	-@erase ".\Debug\Snom-bmp.obj"
	-@erase ".\Debug\Snom-mem.obj"
	-@erase ".\Debug\Dsp-mes.obj"
	-@erase ".\Debug\Snom-dlg.obj"
	-@erase ".\Debug\Snom-avi.obj"
	-@erase ".\Debug\Snom-dsp.obj"
	-@erase ".\Debug\Snom-dat.obj"
	-@erase ".\Debug\snom-prg.obj"
	-@erase ".\Debug\Snomputz.res"
	-@erase ".\Debug\Snom32ms.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

# ADD BASE CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
# ADD CPP /nologo /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /YX /c
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /Zi /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS"\
 /Fp"$(INTDIR)/Snom32ms.pch" /YX /Fo"$(INTDIR)/" /Fd"$(INTDIR)/" /c 
CPP_OBJS=.\Debug/
CPP_SBRS=
# ADD BASE MTL /nologo /D "_DEBUG" /win32
# ADD MTL /nologo /D "_DEBUG" /win32
MTL_PROJ=/nologo /D "_DEBUG" /win32 
# ADD BASE RSC /l 0x407 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
RSC_PROJ=/l 0x409 /fo"$(INTDIR)/Snomputz.res" /d "_DEBUG" 
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
BSC32_FLAGS=/nologo /o"$(OUTDIR)/Snom32ms.bsc" 
BSC32_SBRS=
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no /debug /machine:I386
# SUBTRACT LINK32 /force
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib\
 comctl32.lib ctl3d32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib\
 uuid.lib vfw32.lib /nologo /subsystem:windows /incremental:no\
 /pdb:"$(OUTDIR)/Snom32ms.pdb" /debug /machine:I386\
 /out:"$(OUTDIR)/Snom32ms.exe" 
LINK32_OBJS= \
	"$(INTDIR)/Snom-pac.obj" \
	"$(INTDIR)/Snom-fit.obj" \
	"$(INTDIR)/Snom-mat.obj" \
	"$(INTDIR)/Snomputz.obj" \
	"$(INTDIR)/avi_utils.obj" \
	"$(INTDIR)/Snom-wrk.obj" \
	"$(INTDIR)/Filebox.obj" \
	"$(INTDIR)/Snom-bmp.obj" \
	"$(INTDIR)/Snom-mem.obj" \
	"$(INTDIR)/Dsp-mes.obj" \
	"$(INTDIR)/Snom-dlg.obj" \
	"$(INTDIR)/Snom-avi.obj" \
	"$(INTDIR)/Snom-dsp.obj" \
	"$(INTDIR)/Snom-dat.obj" \
	"$(INTDIR)/snom-prg.obj" \
	"$(INTDIR)/Snomputz.res"

"$(OUTDIR)\Snom32ms.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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

SOURCE=".\Snom-bmp.c"
DEP_CPP_SNOM_=\
	".\Myportab.h"\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Snom-dlg.h"\
	".\Snom-dsp.h"\
	".\Snom-dat.h"\
	".\Snom-wrk.h"\
	".\Snom-mat.h"\
	".\Filebox.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	

"$(INTDIR)\Snom-bmp.obj" : $(SOURCE) $(DEP_CPP_SNOM_) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Filebox.c
DEP_CPP_FILEB=\
	".\Filebox.h"\
	".\Snomputz.h"\
	".\snomlang.h"\
	

"$(INTDIR)\Filebox.obj" : $(SOURCE) $(DEP_CPP_FILEB) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-dat.c"
DEP_CPP_SNOM_D=\
	".\Myportab.h"\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-pac.h"\
	".\Snom-dat.h"\
	".\Snom-mem.h"\
	".\Snom-mat.h"\
	".\Snom-wrk.h"\
	".\Snom-win.h"\
	".\Snom-dlg.h"\
	".\Hdf-file.h"\
	".\Hitachi.h"\
	".\WSxM.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Hntdefs.h"\
	".\Htags.h"\
	

"$(INTDIR)\Snom-dat.obj" : $(SOURCE) $(DEP_CPP_SNOM_D) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-dlg.c"
DEP_CPP_SNOM_DL=\
	".\Myportab.h"\
	".\Filebox.h"\
	".\Snomputz.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-dlg.h"\
	".\Snom-dat.h"\
	".\Snom-dsp.h"\
	".\Snom-mem.h"\
	".\Snom-wrk.h"\
	".\Snom-win.h"\
	".\Snom-mat.h"\
	".\Snom-def.h"\
	".\Snom-typ.h"\
	".\Psi-hdf.h"\
	

"$(INTDIR)\Snom-dlg.obj" : $(SOURCE) $(DEP_CPP_SNOM_DL) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-dsp.c"
DEP_CPP_SNOM_DS=\
	".\Myportab.h"\
	".\Snom-typ.h"\
	".\Snomputz.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-dsp.h"\
	".\Snom-wrk.h"\
	".\Snom-mem.h"\
	".\Snom-mat.h"\
	".\Snom-win.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	

"$(INTDIR)\Snom-dsp.obj" : $(SOURCE) $(DEP_CPP_SNOM_DS) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-fit.c"

!IF  "$(CFG)" == "snom32ms - Win32 Release"

DEP_CPP_SNOM_F=\
	".\Nrutil.h"\
	".\Snom-typ.h"\
	".\Snom-win.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\Snom-fit.obj" : $(SOURCE) $(DEP_CPP_SNOM_F) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

DEP_CPP_SNOM_F=\
	".\Nrutil.h"\
	".\Myportab.h"\
	".\Snom-typ.h"\
	".\Snom-win.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	

"$(INTDIR)\Snom-fit.obj" : $(SOURCE) $(DEP_CPP_SNOM_F) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-mat.c"
DEP_CPP_SNOM_M=\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-fit.h"\
	".\Snom-wrk.h"\
	".\Snom-mem.h"\
	".\Snom-win.h"\
	".\Snom-mat.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\Snom-mat.obj" : $(SOURCE) $(DEP_CPP_SNOM_M) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-mem.c"
DEP_CPP_SNOM_ME=\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\Snom-mem.obj" : $(SOURCE) $(DEP_CPP_SNOM_ME) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-pac.c"
DEP_CPP_SNOM_P=\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-mem.h"\
	".\Snom-win.h"\
	".\Snom-pac.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\Snom-pac.obj" : $(SOURCE) $(DEP_CPP_SNOM_P) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Snomputz.c
DEP_CPP_SNOMP=\
	".\Myportab.h"\
	".\Snomputz.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Filebox.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Snom-dat.h"\
	".\Snom-dlg.h"\
	".\Snom-dsp.h"\
	".\snom-avi.h"\
	".\Dsp-mes.h"\
	".\snom-prg.h"\
	".\Snom-def.h"\
	".\Snom-typ.h"\
	".\Psi-hdf.h"\
	

"$(INTDIR)\Snomputz.obj" : $(SOURCE) $(DEP_CPP_SNOMP) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-wrk.c"
DEP_CPP_SNOM_W=\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-mem.h"\
	".\Snom-win.h"\
	".\Snom-wrk.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\Snom-wrk.obj" : $(SOURCE) $(DEP_CPP_SNOM_W) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Dsp-mes.c"

!IF  "$(CFG)" == "snom32ms - Win32 Release"

DEP_CPP_DSP_M=\
	".\Dsp\Scantyp.h"\
	".\Pc32dll\Target.h"\
	".\Snomputz.h"\
	".\Myportab.h"\
	".\Snom-typ.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-dat.h"\
	".\Snom-dsp.h"\
	".\Snom-mat.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Pc32dll\Cardinfo.h"\
	".\Pc32dll\Alias.h"\
	".\Pc32dll\Ii_iostr.h"\
	".\Pc32dll\Mailbox.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	

"$(INTDIR)\Dsp-mes.obj" : $(SOURCE) $(DEP_CPP_DSP_M) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "snom32ms - Win32 Debug"

DEP_CPP_DSP_M=\
	".\Dsp\Scantyp.h"\
	".\Pc32dll\Target.h"\
	".\Snomputz.h"\
	".\Myportab.h"\
	".\Snom-typ.h"\
	".\Snom-var.h"\
	".\snomlang.h"\
	".\Snom-dat.h"\
	".\Snom-dsp.h"\
	".\Snom-mat.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	

"$(INTDIR)\Dsp-mes.obj" : $(SOURCE) $(DEP_CPP_DSP_M) "$(INTDIR)"


!ENDIF 

# End Source File
################################################################################
# Begin Source File

SOURCE=".\snom-prg.c"
DEP_CPP_SNOM_PR=\
	".\Filebox.h"\
	".\Snomputz.h"\
	".\Snom-typ.h"\
	".\Snom-win.h"\
	".\Snom-mem.h"\
	".\Snom-dlg.h"\
	".\Snom-dsp.h"\
	".\Snom-dat.h"\
	".\Snom-wrk.h"\
	".\Snom-mat.h"\
	".\snom-prg.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Myportab.h"\
	

"$(INTDIR)\snom-prg.obj" : $(SOURCE) $(DEP_CPP_SNOM_PR) "$(INTDIR)"


# End Source File
################################################################################
# Begin Source File

SOURCE=.\Snomputz.rc
DEP_RSC_SNOMPU=\
	".\snomputz.ico"\
	".\snombild.ico"\
	".\toolbar.bmp"\
	".\scanbar.bmp"\
	".\3p.bmp"\
	".\Snomputz.h"\
	".\afxres.h"\
	".\winres.h"\
	

"$(INTDIR)\Snomputz.res" : $(SOURCE) $(DEP_RSC_SNOMPU) "$(INTDIR)"
   $(RSC) $(RSC_PROJ) $(SOURCE)


# End Source File
################################################################################
# Begin Source File

SOURCE=".\Snom-avi.c"
DEP_CPP_SNOM_A=\
	".\Myportab.h"\
	".\Snom-typ.h"\
	".\Snom-var.h"\
	".\Snom-mem.h"\
	".\snomlang.h"\
	".\Snom-dlg.h"\
	".\Snom-dsp.h"\
	".\avi_utils.h"\
	".\Psi-hdf.h"\
	".\Snom-def.h"\
	".\Snomputz.h"\
	

"$(INTDIR)\Snom-avi.obj" : $(SOURCE) $(DEP_CPP_SNOM_A) "$(INTDIR)"


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
