#############################################################################
# FCAS halfwave inject test process (manual start, not in monit)
#############################################################################
PROGRAM   = fcas_inject

CC     = arm-buildroot-linux-gnueabihf-gcc-14.4.0
STRIP  = arm-buildroot-linux-gnueabihf-strip

CFLAGS  = -g -O2 -Wall -I../database -D_GNU_SOURCE
LDFLAGS = -lm

.PHONY: all clean

all: $(PROGRAM)

$(PROGRAM): main.o db_device.o db_fcas_inject.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	$(STRIP) $@

main.o: main.c
	$(CC) $(CFLAGS) -c -o $@ $<

db_device.o: ../database/db_device.c
	$(CC) $(CFLAGS) -c -o $@ $<

db_fcas_inject.o: ../database/db_fcas_inject.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f main.o db_device.o db_fcas_inject.o $(PROGRAM)
