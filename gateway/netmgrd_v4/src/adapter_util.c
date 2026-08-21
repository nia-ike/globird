#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>

static int g_adapter_quiet;

/*********************************************************************************************************
** Function name:       nm4_adapter_set_quiet
** Descriptions:        When set, nm4_run_cmd redirects child stdout/stderr to /dev/null.
** input parameters:    quiet   - non-zero to enable quiet mode.
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_adapter_set_quiet(int quiet)
{
    g_adapter_quiet = quiet ? 1 : 0;
}

/*********************************************************************************************************
** Function name:       nm4_pidfile_alive
** Descriptions:        Check whether a PID file exists and the process is still alive (signal 0).
** input parameters:    path    - path to file containing decimal PID.
** output parameters:   NONE
** Returned value:      1 if alive, 0 if missing, unreadable, or dead.
*********************************************************************************************************/
int nm4_pidfile_alive(const char *path)
{
    FILE *f;
    int pid;

    f = fopen(path, "r");
    if (!f)
        return 0;
    if (fscanf(f, "%d", &pid) != 1) {
        fclose(f);
        return 0;
    }
    fclose(f);
    if (pid <= 0)
        return 0;
    if (kill((pid_t)pid, 0) == 0)
        return 1;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_pppd_running
** Descriptions:        Detect if a pppd instance for the given linkname/ifname is in process list.
** input parameters:    ifname  - PPP interface name (e.g. ppp0).
** output parameters:   NONE
** Returned value:      1 if found, 0 otherwise.
*********************************************************************************************************/
int nm4_pppd_running(const char *ifname)
{
    char cmd[320];

    snprintf(cmd, sizeof(cmd),
             "ps 2>/dev/null | grep -v grep | grep '[p]ppd' | grep -qF '%s'",
             ifname);
    if (system(cmd) == 0)
        return 1;
    return 0;
}

/*********************************************************************************************************
** Function name:       nm4_run_cmd
** Descriptions:        Run a shell command via system(); optionally silence stdout/stderr.
** input parameters:    cmd     - full shell command line.
** output parameters:   NONE
** Returned value:      system() return status (implementation-defined encoding).
*********************************************************************************************************/
int nm4_run_cmd(const char *cmd)
{
    int rc;
    if (g_adapter_quiet) {
        int nullfd = open("/dev/null", O_WRONLY);
        int out_bak = -1;
        int err_bak = -1;
        if (nullfd < 0)
            return system(cmd);
        out_bak = dup(STDOUT_FILENO);
        err_bak = dup(STDERR_FILENO);
        if (out_bak >= 0) (void)dup2(nullfd, STDOUT_FILENO);
        if (err_bak >= 0) (void)dup2(nullfd, STDERR_FILENO);
        close(nullfd);

        rc = system(cmd);

        if (out_bak >= 0) {
            (void)dup2(out_bak, STDOUT_FILENO);
            close(out_bak);
        }
        if (err_bak >= 0) {
            (void)dup2(err_bak, STDERR_FILENO);
            close(err_bak);
        }
        return rc;
    }
    return system(cmd);
}
