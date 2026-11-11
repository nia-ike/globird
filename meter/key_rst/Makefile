#############################################################################
# RST/key2 long-press reset process
#############################################################################
PROGRAM   = key_rst

CC     = arm-buildroot-linux-gnueabihf-gcc-14.4.0
STRIP  = arm-buildroot-linux-gnueabihf-strip

CFLAGS  = -g -O2 -Wall -I../database -D_GNU_SOURCE
LDFLAGS =

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): key_rst.o db_msgevent_api.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

key_rst.o: key_rst.c
	$(CC) $(CFLAGS) -c -o $@ $<

db_msgevent_api.o: ../database/db_msgevent_api.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f key_rst.o db_msgevent_api.o $(PROGRAM)
