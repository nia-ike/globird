all:
	arm-buildroot-linux-gnueabihf-gcc-14.4.0 -o db_process main.c db_cfg_json.c db_cfg_realtime.c db_node_fops.c db_device.c common/db_cp.c common/md5.c -D_GNU_SOURCE -D__USE_XOPEN  -I./lib -L./lib -lcjson -lsqlite3
	arm-buildroot-linux-gnueabihf-strip db_process 

clean:
	rm db_process