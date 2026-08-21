#############################################################################
#
# Generic Makefile for C/C++ Program
#
#############################################################################

PROGRAM   = libDlt698_45.so

CFLAGS_EX =

SYSROOT = /opt/buildroot-gcc-gnueabihf/arm-buildroot-linux-gnueabihf/sysroot
SRCDIRS   =
LIBS_EX   =
CPPFLAGS  = -Wall
LDFLAGS   = --sysroot=$(SYSROOT) -lc -lcjson -lcurl -lsqlite3 -lpthread -fPIC -shared

SOURCES = ../../../database/db_device ../../../database/db_node_fops ../libcommon/libpkt/pkt

SRCEXTS = .c .C .cc .cpp .CPP .c++ .cxx .cp
HDREXTS = .h .H .hh .hpp .HPP .h++ .hxx .hp

CFLAGS  = -g -O0 -I../../../database -I./lib -I../../main/ -I../../main/hal_comm -I../libcommon/libpkt -I../dlt645V07 -I../dlt645V07/libcjson
CXXFLAGS= -g -O0
CC     = arm-buildroot-linux-gnueabihf-gcc
STRIP = arm-buildroot-linux-gnueabihf-strip
CXX    = arm-buildroot-linux-gnueabihf-g++

ETAGS = etags
ETAGSFLAGS =
CTAGS = ctags
CTAGSFLAGS =

SHELL   = /bin/sh
EMPTY   =
SPACE   = $(EMPTY) $(EMPTY)
ifeq ($(PROGRAM),)
  CUR_PATH_NAMES = $(subst /,$(SPACE),$(subst $(SPACE),_,$(CURDIR)))
  PROGRAM = $(word $(words $(CUR_PATH_NAMES)),$(CUR_PATH_NAMES))
  ifeq ($(PROGRAM),)
    PROGRAM = a.out
  endif
endif

ifeq ($(SRCDIRS),)
  SRCDIRS := $(shell find $(SRCDIRS) -type d)
endif

ifeq ($(CFLAGS_EX),)
	CFLAGS_EX := $(foreach d,$(shell find . -type d),-I$(d))
endif

SOURCES += $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
HEADERS = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))
SRC_CXX = $(filter-out %.cpp,$(SOURCES))
OBJS    = $(addsuffix .o, $(basename $(SOURCES)))
DEPS    = $(OBJS:.o=.d)

DEP_OPT = $(shell if `$(CC) --version | grep "GCC" >/dev/null`; then \
                  echo "-MM -MP"; else echo "-M"; fi )
DEPEND      = $(CC)  $(DEP_OPT)  $(CFLAGS_EX) $(CFLAGS) $(CPPFLAGS)
DEPEND.d    = $(subst -g ,,$(DEPEND))
COMPILE.c   = $(CC)  $(CFLAGS_EX) $(CFLAGS)   $(CPPFLAGS) -c
COMPILE.cxx = $(CXX) $(CFLAGS_EX) $(CXXFLAGS) $(CPPFLAGS) -c
LINK.c      = $(CC)  $(CFLAGS_EX) $(CFLAGS)   $(CPPFLAGS) $(LDFLAGS)
LINK.cxx    = $(CC) $(CFLAGS_EX) $(CXXFLAGS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: all objs tags ctags clean distclean help show
.SUFFIXES:

all: $(PROGRAM)
objs:$(OBJS)

%.o:%.c
	$(COMPILE.c) $< -o $@
%.o:%.C
	$(COMPILE.cxx) $< -o $@
%.o:%.cc
	$(COMPILE.cxx) $< -o $@
%.o:%.cpp
	$(COMPILE.cxx) $< -o $@
%.o:%.CPP
	$(COMPILE.cxx) $< -o $@
%.o:%.c++
	$(COMPILE.cxx) $< -o $@
%.o:%.cp
	$(COMPILE.cxx) $< -o $@
%.o:%.cxx
	$(COMPILE.cxx) $< -o $@

tags: $(HEADERS) $(SOURCES)
	$(ETAGS) $(ETAGSFLAGS) $(HEADERS) $(SOURCES)
ctags: $(HEADERS) $(SOURCES)
	$(CTAGS) $(CTAGSFLAGS) $(HEADERS) $(SOURCES)

$(PROGRAM):$(OBJS)
ifeq ($(SRC_CXX),)
	$(LINK.c)   $(OBJS) $(LIBS_EX) -o $@
	$(STRIP) $@
	@echo Type ./$@ to execute the program.
else
	$(LINK.cxx) $(OBJS) $(LIBS_EX) -o $@
	$(STRIP) $@
	@echo Type ./$@ to execute the program.
endif

ifndef NODEP
ifneq ($(DEPS),)
  sinclude $(DEPS)
endif
endif

clean:
	$(RM) $(OBJS) $(DEPS) $(PROGRAM) $(PROGRAM).exe

distclean: clean
	$(RM) $(DEPS) TAGS
