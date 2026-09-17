# Makefile for netmgrd_v4（与 netmgrd_v3 同一套交叉链与 sysroot）
#
# 默认：ARM 软浮/uClibc 目标（在设备上可运行）。勿用宿主机 gcc 直编后拷到板子。
# 本机只测语法/快速冒烟：  make clean && make HOST=1
#
# 若曾误用 x86 编译，务必先  make clean  再全量重编，否则会混入错误架构的 .o
HOST ?= 0
ifeq ($(HOST),1)
  CC      := gcc
  STRIP   := strip
  SYSROOT :=
  $(info netmgrd_v4: HOST=1 — 生成 **本机** x86/amd64 等架构，**不可**在 ARM 板运行)
else
  CC      := arm-buildroot-linux-gnueabihf-gcc-14.4.0
  STRIP   := arm-buildroot-linux-gnueabihf-strip
  SYSROOT := /opt/buildroot-gcc-gnueabihf14.4.0/arm-buildroot-linux-gnueabihf/sysroot
  $(info netmgrd_v4: 使用 $(CC) + sysroot — 供 ARM 目标机)
endif

METER_DB_DIR = ../../meter/database

CFLAGS = -Wall -Wextra -std=c11 -O2 -D_GNU_SOURCE -D__USE_XOPEN \
         -I./include \
         -I./src/lib \
         -I$(METER_DB_DIR)

ifneq ($(strip $(SYSROOT)),)
CFLAGS += --sysroot=$(SYSROOT)
LDFLAGS += --sysroot=$(SYSROOT)
endif

LDFLAGS +=

SRC_DIR = src
OBJ_DIR = build/obj
BIN_DIR = build/bin

SRCS = $(SRC_DIR)/main.c \
       $(SRC_DIR)/log.c \
       $(SRC_DIR)/config_v3json.c \
       $(SRC_DIR)/config_networkjson.c \
       $(SRC_DIR)/config_apply.c \
       $(SRC_DIR)/cJSON.c \
       $(SRC_DIR)/gateway.c \
       $(SRC_DIR)/probe.c \
       $(SRC_DIR)/policy.c \
       $(SRC_DIR)/iface_avail.c \
       $(SRC_DIR)/route.c \
       $(SRC_DIR)/linkkeeper.c \
       $(SRC_DIR)/hw_sim.c \
       $(SRC_DIR)/manager.c \
       $(SRC_DIR)/status.c \
       $(SRC_DIR)/adapter_util.c \
       $(SRC_DIR)/adapter_wpa.c \
       $(SRC_DIR)/adapter_ppp.c \
       $(SRC_DIR)/dns.c \
       $(SRC_DIR)/led.c \
       $(SRC_DIR)/wifi_usb_watch.c \
       $(SRC_DIR)/reboot.c

OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SRCS)) \
       $(OBJ_DIR)/db_msgevent_api.o
TARGET = $(BIN_DIR)/netmgrd

.PHONY: all clean install strip debug print

all: $(TARGET)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR)/db_msgevent_api.o: $(METER_DB_DIR)/db_msgevent_api.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "Built: $(TARGET)"
	@command -v file >/dev/null 2>&1 && file "$(TARGET)" || true
ifneq ($(HOST),1)
	@if command -v file >/dev/null 2>&1; then \
		file "$(TARGET)" | grep -qE 'ARM|arm' || \
		( echo "netmgrd_v4: 该 ELF 不像 ARM，无法在板端运行。请 make clean 后用 $(CC) + SYSROOT 重编，勿用 HOST=1/cc"; \
		  file "$(TARGET)"; exit 1 ); \
	fi
endif

strip: $(TARGET)
	$(STRIP) $(TARGET)
	@echo "Stripped: $(TARGET)"

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Cleaned build files"

# 安装（可选）
install: $(TARGET)
	install -d $(DESTDIR)/usr/bin
	install -m 755 $(TARGET) $(DESTDIR)/usr/bin/

debug: CFLAGS += -g -O0 -DDEBUG
debug: $(TARGET)

print:
	@echo "HOST: $(HOST)  (1=本机 gcc，0=ARM 交叉链)"
	@echo "CC: $(CC)"
	@echo "STRIP: $(STRIP)"
	@echo "SYSROOT: $(SYSROOT)"
	@echo "CFLAGS: $(CFLAGS)"
	@echo "LDFLAGS: $(LDFLAGS)"
	@echo "Sources: $(SRCS)"
	@echo "Objects: $(OBJS)"
	@echo "Target: $(TARGET)"
