PROGRAM:=$(shell ls -d  */)

all:
	if [ -d lib ]; then \
		rm -r lib; \
	fi \
	
	mkdir lib
	
	for file in $(PROGRAM) ; \
	do \
		make -C $$file -j 24;  \
	done \

	cp dlt645V07/libDlt645_07.so lib/
	cp dlt698-45/libDlt698_45.so lib/
	cp cjt188/libCjt188.so lib/
	cp modbusRTU/libmodbusRTU.so lib/
	cp modbusTCP/libmodbusTCP.so lib/
	
clean:
	for file in $(PROGRAM) ; \
	do \
	make clean -C $$file;  \
	done \

help:
	@echo "make all the program in this path"
