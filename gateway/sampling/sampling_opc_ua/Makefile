PROGRAM   = sampling_opc_ua

SYSROOT = /opt/buildroot-gcc-gnueabihf/arm-buildroot-linux-gnueabihf/sysroot
SRCDIRS   = . config opc_ua_client
LIBS_EX   = -L../../database -L../../../buildroot/output/target/usr/lib
CPPFLAGS  = -Wall -Wno-unused-function
LDFLAGS   = --sysroot=$(SYSROOT) -lc -lcjson -lpthread -lopen62541

SOURCES = ../../database/db_device ../../database/db_node_fops ../../common/dev_log

SRCEXTS = .c .C .cc .cpp .CPP .c++ .cxx .cp
HDREXTS = .h .H .hh .hpp .HPP .h++ .hxx .hp

CFLAGS  = -g -O0 \
          -I../../database \
          -I../../common \
          -I../sampling_cs104/common \
          -I../sampling_cs104/lib/libcjson \
          -I$(SYSROOT)/usr/include

CC     = arm-buildroot-linux-gnueabihf-gcc
STRIP  = arm-buildroot-linux-gnueabihf-strip

SHELL   = /bin/sh
EMPTY   =
SPACE   = $(EMPTY) $(EMPTY)

ifeq ($(CFLAGS_EX),)
	CFLAGS_EX := $(foreach d,$(shell find . -type d),-I$(d))
endif

SOURCES += $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
HEADERS = $(foreach d,$(shell find . -type d),$(wildcard $(addprefix $(d)/*,$(HDREXTS))))
OBJS    = $(addsuffix .o, $(basename $(SOURCES)))

COMPILE.c = $(CC) $(CFLAGS_EX) $(CFLAGS) $(CPPFLAGS) -c
LINK.c    = $(CC) $(CFLAGS_EX) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS)

.PHONY: all clean

all: $(PROGRAM)

%.o:%.c
	$(COMPILE.c) $< -o $@

$(PROGRAM): $(OBJS)
	$(LINK.c) $(OBJS) $(LIBS_EX) -o $@
	$(STRIP) $@
	@echo Type ./$@ to execute the program.

clean:
	$(RM) $(OBJS) $(PROGRAM)
