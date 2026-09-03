/* SIM 物理插卡检测（/dev/input/event1）与 LTE 模组重启 */
#ifndef NETMGR_V4_HW_SIM_H
#define NETMGR_V4_HW_SIM_H

#include <stdint.h>

#include "netmgr_v4_linkkeeper.h"

#define NM4_SIM_PRESENT_PATH "/var/run/netmgr/sim.present"

typedef enum {
    NM4_SIM_EDGE_NONE = 0,
    NM4_SIM_EDGE_CHANGE
} nm4_sim_edge_t;

/*********************************************************************************************************
** Function name:       nm4_sim_init
** Descriptions:        Open event device; read initial SIM state; write sim.present cache.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      0 on success, -1 if event device missing (poll-only fallback).
*********************************************************************************************************/
int nm4_sim_init(void);

/*********************************************************************************************************
** Function name:       nm4_sim_shutdown
** Descriptions:        Close event fd.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      NONE
*********************************************************************************************************/
void nm4_sim_shutdown(void);

/*********************************************************************************************************
** Function name:       nm4_sim_present
** Descriptions:        Read SIM slot via EVIOCGKEY (same as sync_statusinfo sysinfo_sim_detect).
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      1 inserted, 0 empty, -1 detect failed.
*********************************************************************************************************/
int nm4_sim_present(void);

/*********************************************************************************************************
** Function name:       nm4_sim_poll
** Descriptions:        Poll event1 + steady state; hysteresis merges bursts into one edge.
** input parameters:    now_ms  - current time for debounce window.
** output parameters:   NONE
** Returned value:      NM4_SIM_EDGE_CHANGE when activity and outside debounce window.
*********************************************************************************************************/
nm4_sim_edge_t nm4_sim_poll(uint64_t now_ms);

/*********************************************************************************************************
** Function name:       nm4_sim_last_present
** Descriptions:        Last cached steady SIM state from poll/init.
** input parameters:    NONE
** output parameters:   NONE
** Returned value:      1 / 0 / -1
*********************************************************************************************************/
int nm4_sim_last_present(void);

/*********************************************************************************************************
** Function name:       nm4_sim_debounce_active
** Descriptions:        Whether SIM activity is inside post-trigger hysteresis window.
** input parameters:    now_ms  - current time.
** output parameters:   NONE
** Returned value:      1 if inside window, else 0.
*********************************************************************************************************/
int nm4_sim_debounce_active(uint64_t now_ms);

/*********************************************************************************************************
** Function name:       nm4_sim_ppp_effective
** Descriptions:        PPP runtime enabled: JSON has_ppp and SIM physically present.
** input parameters:    lk      - linkkeeper (has_ppp flag).
** output parameters:   NONE
** Returned value:      1 if PPP flows should run, else 0.
*********************************************************************************************************/
int nm4_sim_ppp_effective(const nm4_linkkeeper_t *lk);

/*********************************************************************************************************
** Function name:       nm4_sim_on_edge
** Descriptions:        Debounced SIM activity: stop ppp, lte_ctrl restart, arbitrate.
** input parameters:    lk      - linkkeeper state (may be NULL).
**                      now_ms  - wall time for timestamps.
** output parameters:   lk      - ppp wait flags updated when non-NULL.
** Returned value:      0 on restart success, -1 on failure.
*********************************************************************************************************/
int nm4_sim_on_edge(nm4_linkkeeper_t *lk, uint64_t now_ms);

#endif
