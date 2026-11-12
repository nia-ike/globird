PROGRAM:=$(shell ls -d  */)

all:
	for file in $(PROGRAM) ; \
	do \
	make clean -C $$file;  \
	make -C $$file -j 24;  \
	done \

	#rm -r bin
	mkdir -p bin
	
	cp database/db_process_meter bin/
	cp measure_process/measure_process_rn7302 bin/
	cp modbus_slave_meter/modbus_slave_meter bin/
	cp fcas_inject/fcas_inject bin/
	


clean:
	for file in $(PROGRAM) ; \
	do \
	make clean -C $$file;  \
	done \

help:
	@echo "make all the program in this path"
