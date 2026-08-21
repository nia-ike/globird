/*
 * sampling_bypass.h
 * 临时串口旁路：TCP Server <-> 已打开的串口 FD（精简自 iobridge）
 */
#ifndef SAMPLING_BYPASS_H
#define SAMPLING_BYPASS_H

#include "sampling_channelpoll.h"

/**
 * 若全局 debug.bypass.chnl 命中本通道且为串口链路，则阻塞运行旁路直至关闭/超时。
 * @return 1 若执行了旁路会话；0 未命中（调用方继续 poll）
 */
int ChnnlPoll_BypassTryRun(struct channelpoll_mgr_t *pChnlPollMgr);

#endif
