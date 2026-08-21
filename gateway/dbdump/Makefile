all:
	arm-buildroot-linux-gnueabihf-gcc -o dbdump main.c ../database/db_node_fops.c ../database/db_device.c common/db_cp.c common/md5.c -D_GNU_SOURCE -D__USE_XOPEN  -I./lib -I../database -L./lib -lcjson -lsqlite3
	arm-buildroot-linux-gnueabihf-strip dbdump 

clean:
	rm dbdump