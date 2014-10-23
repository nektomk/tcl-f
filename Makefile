OBJS=tclf.o map.o inspect.o tuple.o cons.o lazy.o
TARGET=tclf
DISTDIR=./f

INCLUDE=-Iinclude

CFLAGS=$(INCLUDE) 
# C warnings
CFLAGS+=-Wall -Wextra  -Wno-strict-aliasing 
# optimization flags
CFLAGS+=-fomit-frame-pointer -O3 
# build as Tcl STUBbed extension
CFLAGS+=-DUSE_TCL_STUBS=1

## OS SPECIFIC
SYS := $(shell gcc -dumpmachine)
ifneq (, $(findstring mingw, $(SYS)))
 # Do Mingw
MINGW=C:/MinGW
DLLSO=dll
LD=gcc
STUB=$(MINGW)/lib/libtclstub86.a
%.dll : %.o
		$(LD) $(LDFLAGS) -shared -fPIC -o $@ $^ $(LDLIBS) $(STUB) -static-libgcc
endif
ifneq (, $(findstring linux, $(SYS)))
endif 
ifneq (, $(findstring cygwin, $(SYS)))
else
endif

VPATH=src

all: tclf.$(DLLSO)

$(TARGET).$(DLLSO): $(OBJS)

dist: $(TARGET).$(DLLSO)
		cp -f $(TARGET).$(DLLSO) $(DISTDIR)

.PHONY: clean distclean touch

clean:
		$(RM) *.o
		$(RM) *.dll
		$(RM) *.so

distclean:
		$(RM) $(DISTDIR)/$(TARGET).dll
		$(RM) $(DISTDIR)/$(TARGET).so

touch:
		touch -c *.c
		touch -c *.h





