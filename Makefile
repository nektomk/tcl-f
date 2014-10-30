OBJS=f.o map.o inspect.o tuple.o cons.o lazy.o iter.o func.o call.o chain.o
TARGET=f
DISTDIR=./f

#INCLUDE=-Isrc

CFLAGS=$(TCL_INCLUDE_SPEC) -Iinclude
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
TCL_CONFIG=C:/MinGW/lib/tclConfig.sh
include $(TCL_CONFIG)
MINGW=C:/MinGW
DLLSO=dll
LD=gcc
STUB=$(MINGW)/lib/libtclstub86.a
%.dll : %.o
		$(LD) $(LDFLAGS) -shared -fPIC -o $@ $^ $(LDLIBS) $(TCL_STUB_LIB_PATH) -static-libgcc
endif
ifneq (, $(findstring linux, $(SYS)))
endif 
ifneq (, $(findstring cygwin, $(SYS)))
else
endif

VPATH=src

all: $(TARGET).$(DLLSO) 

$(TARGET).$(DLLSO): $(OBJS)

dist: $(TARGET).$(DLLSO)
		cp -f $(TARGET).$(DLLSO) $(DISTDIR)

.PHONY: clean distclean touch test test-func test-lazy test-tuple test-call test-chain test-map test-filter test-fold

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

test:
		cd test;tclsh all.tcl

test-func:
		cd test;tclsh all.tcl -match func-*

test-call:
		cd test;tclsh all.tcl -match call-*

test-lazy:
		cd test;tclsh all.tcl -match lazy-*

test-tuple:
		cd test;tclsh all.tcl -match tuple-*

test-chain:
		cd test;tclsh all.tcl -match chain-*

test-map:
		cd test;tclsh all.tcl -match map-*

test-filter:
		cd test;tclsh all.tcl -match filter-*

test-fold:
		cd test;tclsh all.tcl -match filter-*