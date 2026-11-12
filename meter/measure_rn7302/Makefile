#############################################################################
# msr_rn7302：测量进程
# 用法：
#   make -C measure_rn7302
#   make -C measure_rn7302 clean
# 传参与 measure_process 相同：-d -t -c -r -n -h
#############################################################################

CC      = arm-buildroot-linux-gnueabihf-gcc-14.4.0
STRIP   = arm-buildroot-linux-gnueabihf-strip
CPPFLAGS = -Wall
CFLAGS  = -g -O2 \
  -ffunction-sections -fdata-sections \
  -mcpu=cortex-a7 -mfpu=neon-vfpv4 -mfloat-abi=hard \
  -ffast-math -fPIC
LDFLAGS = -lm -pthread -Wl,--gc-sections -flto

PROGRAM = msr_rn7302

SRCDIRS := $(shell find . -type d)
INCLUDES := $(foreach d,$(SRCDIRS),-I$(d)) -I../database
SOURCES := $(foreach d,$(SRCDIRS),$(wildcard $(d)/*.c))
OBJS    := $(SOURCES:.c=.o)

# 与 measure_process 相同的 database 链接单元
DB_SRCS := \
  ../database/db_device.c \
  ../database/db_msr_msgevent_api.c \
  ../database/db_msgevent_api.c \
  ../database/db_measure_info.c \
  ../database/db_fcas_inject.c
DB_OBJS := $(notdir $(DB_SRCS:.c=.o))

# 关闭 make 内置「用 .o 链接同名可执行文件」规则，避免误链
.SUFFIXES:
%:: %.o
%:: %.c

.PHONY: all clean show

all: $(PROGRAM)

%.o: %.c
	$(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(DB_OBJS): %.o: ../database/%.c
	$(CC) $(INCLUDES) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(PROGRAM): $(OBJS) $(DB_OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJS) $(DB_OBJS) -o $@
	$(STRIP) $@

clean:
	$(RM) $(OBJS) $(DB_OBJS) $(PROGRAM)

show:
	@echo 'PROGRAM :' $(PROGRAM)
	@echo 'SOURCES :' $(SOURCES)
	@echo 'DB_SRCS :' $(DB_SRCS)
	@echo 'OBJS    :' $(OBJS)
	@echo 'DB_OBJS :' $(DB_OBJS)
	@echo 'INCLUDES:' $(INCLUDES)
