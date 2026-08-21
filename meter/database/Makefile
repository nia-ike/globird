all:
	arm-buildroot-linux-gnueabihf-gcc-14.4.0 -o db_process_meter -ffunction-sections -fdata-sections main.c db_cfg_json.c db_device.c db_cfg.c db_cfg_sys.c db_cfg_user_base.c db_cfg_user_multirate.c db_cfg_user_expand.c db_energy_realtime.c db_energy_history.c db_energy_multirate.c db_energy_runing.c db_maxDMD.c db_msg_process.c db_msgevent_api.c db_msr_msgevent_api.c db_cp.c db_cfg_info.c db_loadrun.c md5.c -D_GNU_SOURCE -D__USE_XOPEN -I./lib -L./lib -lcjson -lsqlite3
	arm-buildroot-linux-gnueabihf-strip db_process_meter 

clean:
	rm *.o
	rm db_process_meter