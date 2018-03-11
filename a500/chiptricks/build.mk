TOPDIR = $(realpath $(dir $(lastword $(MAKEFILE_LIST)))/..)

LIBS += libsys libc
CPPFLAGS += -I$(TOPDIR)/effects
LDEXTRA = $(foreach lib,$(LIBS),$(TOPDIR)/base/$(lib)/$(lib).a)
STARTUP = $(TOPDIR)/effects/startup.o 

include $(TOPDIR)/build.mk

%.exe: $(CRT0) %.o $(OBJECTS) $(STARTUP) $(LDEXTRA)
	@echo "[$(DIR):ld] $@"
	$(CC) $(LDFLAGS) -Wl,-Map=$@.map -o $@ $^ $(LDLIBS)
	$(CP) $@ $@.dbg
	$(STRIP) $@

%.3d: %.lwo
	@echo "[$(DIR):conv] $< -> $@"
	$(DUMPLWO) $< $@

%.ilbm: %.png
	@echo "[$(DIR):conv] $< -> $@"
	$(ILBMCONV) $< $@
	$(ILBMPACK) -f $@

%.bin: %.asm
	@echo "[$(DIR):bin] $< -> $@"
	$(AS) -Fbin -o $@ $<

%.adf: %.exe $(DATA)
	@echo "[$(DIR):adf] $^ -> $@"
	$(FSUTIL) -b $(TOPDIR)/base/bootloader.bin create $@ $^

run: all $(notdir $(PWD)).adf
	$(RUNINUAE) -e $(notdir $(PWD)).exe.dbg $(lastword $^)

clean::
	@$(RM) *.adf *.exe *.exe.dbg *.exe.map

.PHONY: run
.PRECIOUS: %.o
