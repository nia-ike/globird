#ifndef ADAPTER_INTERNAL_H
#define ADAPTER_INTERNAL_H

/*********************************************************************************************************
** Function name:       nm4_pidfile_alive
** Descriptions:        Return whether PID file exists and process is alive; see src/adapter_util.c.
** input parameters:    path    - pidfile path.
** output parameters:   NONE
** Returned value:      1 if alive, 0 otherwise.
*********************************************************************************************************/
int nm4_pidfile_alive(const char *path);

/*********************************************************************************************************
** Function name:       nm4_pppd_running
** Descriptions:        Detect pppd in ps for given ifname; see src/adapter_util.c.
** input parameters:    ifname  - PPP interface / linkname.
** output parameters:   NONE
** Returned value:      1 if running, 0 otherwise.
*********************************************************************************************************/
int nm4_pppd_running(const char *ifname);

/*********************************************************************************************************
** Function name:       nm4_run_cmd
** Descriptions:        Run shell command, optionally quiet stdout/stderr; see src/adapter_util.c.
** input parameters:    cmd     - shell command line.
** output parameters:   NONE
** Returned value:      system() status.
*********************************************************************************************************/
int nm4_run_cmd(const char *cmd);

/*********************************************************************************************************
** Function name:       nm4_adapter_set_quiet
** Descriptions:        Enable/disable quiet mode for nm4_run_cmd; see src/adapter_util.c.
** input parameters:    quiet   - non-zero to redirect child output to /dev/null.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_adapter_set_quiet(int quiet);

int nm4_if_has_ipv4(const char *ifname);

#endif
