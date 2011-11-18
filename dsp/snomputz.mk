#
# Innovative Integration, Inc
# Auto-generated makefile
#

#
# Clear suffix list & use a new one
#
.SUFFIXES: 
.SUFFIXES: .out .obj .c .asm .cmd .lib .h .a0

#
# Macros
#
RESPONSE = snomputz.rsp
CMD = C:\SPM\PC32cc\generic.cmd

#
# Generic tools file for *.MAK
#

#
# Macros
#
ASM = asm30
ASM_ARGS = -v30 -s 
CC = cl30
CC_ARGS = -g -mn -v30 -q -x2 -o2
LNK = lnk30
LNK_ARGS = -c -stack 0x200 -heap 0x400 
AR = ar30
AR_ARGS = -r
HEX = hex30
HEX_ARGS = prom.cmd
HEX2BIN = hex2bin
HEX2BIN_ARGS = -f
HEXVERT = hexvert
HEXVERT_ARGS = -w4 -r   
LIBS = -l stdio.lib -l periph.lib -l dsp.lib -l rts30.lib
DEBUGGER = c:\spm\composer\cc_app.exe
# EXECUTE = terminal.exe

# Uncomment to explicitly set output object file
# OUTPUT = filename.out

OUTPUT_BASE = snomputz
OUTPUT = snomputz.out
#
#  Files for project
#
MAKEDEPSRC_ASM =
MAKEDEPSRC = pid.c scan.c

#
#  Targets and dependencies
#
build: $(OUTPUT)
pid.obj: pid.c scantyp.h pid.h snomputz.mki $(FRC)
scan.obj: scan.c scantyp.h pid.h snomputz.mki $(FRC)
rebuild: 
	$(MAKE) build FRC=force_rebuild -f snomputz.mk
force_rebuild:

#
#  Build rules
#
$(OUTPUT_BASE).out : $(MAKEDEPSRC:.c=.obj) $(MAKEDEPSRC_ASM:.asm=.obj)\
                     $(CMD)
		$(LNK) -o $@ $(RESPONSE)
$(OUTPUT_BASE).lib : $(MAKEDEPSRC:.c=.obj) $(MAKEDEPSRC_ASM:.asm=.obj)
		!$(AR) $(AR_ARGS) $@ $?
$(OUTPUT_BASE).a0 : $(OUTPUT_BASE).out
$(OUTPUT_BASE).hh : $(OUTPUT_BASE).a0
		!$(HEX2BIN) -f$*.a0
		!$(HEXVERT) -f$*.bin $(HEXVERT_ARGS)
		@copy $*.txt $*.hh
		@erase $*.txt
# 
# Inference rules...
#    
.c.obj:
    $(CC) $(CC_ARGS) -fr $(<D) $<

.asm.obj:
    $(ASM) $(ASM_ARGS) $<

.out.a0:
    $(HEX) $< $(HEX_ARGS)

