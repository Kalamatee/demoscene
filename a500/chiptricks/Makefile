CPPFLAGS = -I../include -I../effects
LIBS = libsys libahx

LIBFILES = $(foreach lib,$(LIBS),../$(lib)/$(lib).a)

chiptricks.exe: ct-load.o ct-gui.o ct-scope.o chiptricks.o

chiptricks.7z: chiptricks.exe
	7z a "chiptricks-$$(date +%F-%H%M).7z" $< chiptricks

define emit_lib_rule
$(1):
	@echo "MAKE $(notdir $(1))"
	make -C $(dir $(1))
endef

$(foreach lib,$(LIBFILES),$(eval $(call emit_lib_rule,$(lib))))

%.exe: ../crt0.o %.o ../effects/startup.o $(LIBFILES)
	@echo "LINK $@"
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: $(LIBFILES)

include ../Makefile.common
