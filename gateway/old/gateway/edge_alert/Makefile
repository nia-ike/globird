#
# Copyright (c) 2024
# All rights reserved.
#
# 文件名称：Makefile
# 文件标识：
# 描    述：edge_alert 告警服务编译规则
#
# 当前版本：V1.0
# 作    者：chenzy
# 更改说明：创建
# 完成日期：20240702
#

PROGRAM   = edge_alert

CFLAGS_EX =

SYSROOT = /opt/buildroot-gcc-gnueabihf14.4.0/arm-buildroot-linux-gnueabihf/sysroot
SRCDIRS   =
LIBS_EX   =
CPPFLAGS  = -Wall -Wno-unused-function

LDFLAGS   = -lc -lcjson -lpthread -lsqlite3 -lm
ifneq ($(SYSROOT),)
  LDFLAGS := --sysroot=$(SYSROOT) $(LDFLAGS)
  CFLAGS_EX += -I$(SYSROOT)/usr/include
endif

SOURCES   = ../database/db_device ../database/db_node_fops ../common/dev_log

SRCEXTS = .c .C .cc .cpp .CPP .c++ .cxx .cp
HDREXTS = .h .H .hh .hpp .HPP .h++ .hxx .hp

CFLAGS  = -g -O0 -I. -I../common -I../database -I../database/lib
CXXFLAGS= -g -O0

CC     = arm-buildroot-linux-gnueabihf-gcc-14.4.0
STRIP  = arm-buildroot-linux-gnueabihf-strip
CXX    = arm-buildroot-linux-gnueabihf-g++

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

.PHONY: all objs clean distclean help show

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

$(PROGRAM):$(OBJS)
ifeq ($(SRC_CXX),)
	$(LINK.c)   $(OBJS) $(LIBS_EX) -o $@
	$(STRIP) $@
else
	$(LINK.cxx) $(OBJS) $(LIBS_EX) -o $@
	$(STRIP) $@
endif

clean:
	$(RM) $(OBJS) $(DEPS) $(PROGRAM) $(PROGRAM).exe

distclean: clean
	$(RM) $(DEPS) TAGS

help:
	@echo "make -C edge_alert  build edge_alert"

show:
	@echo 'PROGRAM     :' $(PROGRAM)
	@echo 'SOURCES     :' $(SOURCES)
	@echo 'OBJS        :' $(OBJS)
