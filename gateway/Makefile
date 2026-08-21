PROGRAM:=$(shell ls -d  */)

all:
	for file in $(PROGRAM) ; \
	do \
		if [ "$$file" = "strategy_control/" ]; then \
			for subdir in strategy_control/*/ ; do \
				[ -d "$$subdir" ] || continue; \
				$(MAKE) clean -C "$$subdir"; \
				$(MAKE) -C "$$subdir" -j 24; \
			done; \
		elif [ "$$file" = "sampling/" ]; then \
			make clean -C $$file/main; \
			make -C $$file/main -j 24; \
			make -C $$file/dll -j 24; \
			make -C $$file/sampling_cs104 -j 24; \
			make -C $$file/sampling_opc_ua -j 24; \
		else \
			make clean -C $$file; \
			make -C $$file -j 24; \
		fi \
	done \

	#rm -r bin
	mkdir -p bin

	cp database/db_process bin/
	cp datacache/data_cache bin/
	cp dbdump/dbdump bin/
	cp modbus_slave/modbus_slave bin/
	cp sampling/main/sampling_service bin/
	cp sampling/sampling_cs104/sampling_iec104 bin/
	cp sampling/sampling_opc_ua/sampling_opc_ua bin/
	cp sync_statusinfo/sync_statusinfo bin/
	cp key_reset/key_reset bin/
	cp remote_manage_v2/remote_manage bin/
	cp iec104_slave/iec104_slave bin/
	cp opc_ua_slave/opc_ua_slave bin/
	cp iobridge/iobridge bin/
	cp edge_script/edge_script_service bin/
	cp edge_alert/edge_alert bin/
	
	for subdir in strategy_control/*/ ; do \
		name=$$(basename "$$subdir"); \
		[ -f "$$subdir/$$name" ] && cp "$$subdir/$$name" bin/; \
	done

	
clean:
	for file in $(PROGRAM) ; \
	do \
		if [ "$$file" = "strategy_control/" ]; then \
			for subdir in strategy_control/*/ ; do \
				[ -d "$$subdir" ] || continue; \
				$(MAKE) clean -C "$$subdir"; \
			done; \
		else \
			make clean -C $$file; \
		fi \
	done \

help:
	@echo "make all the program in this path"
