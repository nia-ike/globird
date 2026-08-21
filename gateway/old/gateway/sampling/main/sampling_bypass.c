/*
 * sampling_bypass.c
 * 临时调试透传：TCP Server <-> serial（仅 COM1~8）
 */

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "sampling_bypass.h"
#include "db_node_fops.h"

#define BYPASS_PUMP_BUF		1460
#define BYPASS_SELECT_MS	200

static int bypass_set_nonblock(int fd)
{
	int flags;

	if (fd < 0)
		return -1;
	flags = fcntl(fd, F_GETFL, 0);
	if (flags < 0)
		return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int bypass_listen_open(uint16_t port)
{
	int fd;
	int on = 1;
	struct sockaddr_in addr;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return -1;

	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		close(fd);
		return -1;
	}
	if (listen(fd, 1) < 0) {
		close(fd);
		return -1;
	}
	bypass_set_nonblock(fd);
	return fd;
}

static int bypass_still_mine(struct db_spl_bypass_ctl_t *pb, int32_t my_chnl)
{
	time_t now;

	if (pb == NULL)
		return 0;
	if (pb->chnl != my_chnl)
		return 0;
	now = time(NULL);
	if (pb->deadline != 0 && (uint32_t)now >= pb->deadline) {
		pb->chnl = 0;
		pb->status = DB_BYPASS_STATUS_IDLE;
		pb->seq++;
		return 0;
	}
	return 1;
}

static void bypass_close_fd(int *pfd)
{
	if (pfd && *pfd >= 0) {
		close(*pfd);
		*pfd = -1;
	}
}

/**
 * 阻塞旁路会话：命中则占用本线程直到退出条件。
 */
int ChnnlPoll_BypassTryRun(struct channelpoll_mgr_t *pChnlPollMgr)
{
	struct db_spl_bypass_ctl_t *pb;
	int32_t my_chnl;
	int listen_fd = -1;
	int client_fd = -1;
	int serial_fd;
	uint8_t buf[BYPASS_PUMP_BUF];
	uint32_t enter_seq;

	if (pChnlPollMgr == NULL || pChnlPollMgr->pVmem == NULL || pChnlPollMgr->pBaseComm == NULL)
		return 0;

	/* 仅串口通道 */
	if (pChnlPollMgr->pChannelAttr == NULL ||
	    strcmp(pChnlPollMgr->pChannelAttr->working_attr.link, "serial") != 0)
		return 0;

	my_chnl = pChnlPollMgr->ChannelNo & 0x3f;
	if (my_chnl < DB_BYPASS_SERIAL_CHNL_MIN || my_chnl > DB_BYPASS_SERIAL_CHNL_MAX)
		return 0;

	pb = db_bypass_ctl(pChnlPollMgr->pVmem);
	if (pb == NULL || !bypass_still_mine(pb, my_chnl))
		return 0;

	serial_fd = pChnlPollMgr->com_ctx.fd;
	if (serial_fd < 0)
		return 0;

	enter_seq = pb->seq;
	pChnlPollMgr->pBaseComm->flush(&pChnlPollMgr->com_ctx);

	listen_fd = bypass_listen_open(pb->listen_port);
	if (listen_fd < 0) {
		pb->status = DB_BYPASS_STATUS_FAIL;
		fprintf(stderr, "[bypass] chnl %d listen :%u fail errno=%d\n",
			(int)my_chnl, (unsigned)pb->listen_port, errno);
		pb->chnl = 0;
		pb->seq++;
		pb->status = DB_BYPASS_STATUS_IDLE;
		return 1;
	}

	pb->status = DB_BYPASS_STATUS_LISTEN;
	fprintf(stderr, "[bypass] chnl %d TCP server :%u active (lease until %u)\n",
		(int)my_chnl, (unsigned)pb->listen_port, pb->deadline);

	while (bypass_still_mine(pb, my_chnl) && pb->seq == enter_seq) {
		fd_set rd;
		int maxfd;
		struct timeval tv;
		int n;

		FD_ZERO(&rd);
		FD_SET(listen_fd, &rd);
		maxfd = listen_fd;
		FD_SET(serial_fd, &rd);
		if (serial_fd > maxfd)
			maxfd = serial_fd;
		if (client_fd >= 0) {
			FD_SET(client_fd, &rd);
			if (client_fd > maxfd)
				maxfd = client_fd;
		}

		tv.tv_sec = 0;
		tv.tv_usec = BYPASS_SELECT_MS * 1000;
		n = select(maxfd + 1, &rd, NULL, NULL, &tv);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			pb->status = DB_BYPASS_STATUS_FAIL;
			break;
		}
		if (n == 0)
			continue;

		/* 新 TCP 连接：单客户端，挤掉旧连接 */
		if (FD_ISSET(listen_fd, &rd)) {
			struct sockaddr_in peer;
			socklen_t plen = sizeof(peer);
			int nfd = accept(listen_fd, (struct sockaddr *)&peer, &plen);
			if (nfd >= 0) {
				int on = 1;
				bypass_close_fd(&client_fd);
				setsockopt(nfd, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on));
				bypass_set_nonblock(nfd);
				client_fd = nfd;
				pb->status = DB_BYPASS_STATUS_ACTIVE;
			}
		}

		/* TCP -> serial */
		if (client_fd >= 0 && FD_ISSET(client_fd, &rd)) {
			ssize_t nr = read(client_fd, buf, sizeof(buf));
			if (nr > 0) {
				ssize_t nw = write(serial_fd, buf, (size_t)nr);
				(void)nw;
			} else if (nr == 0 || (nr < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
				bypass_close_fd(&client_fd);
				pb->status = DB_BYPASS_STATUS_LISTEN;
			}
		}

		/* serial -> TCP */
		if (FD_ISSET(serial_fd, &rd)) {
			ssize_t nr = read(serial_fd, buf, sizeof(buf));
			if (nr > 0) {
				if (client_fd >= 0) {
					ssize_t nw = write(client_fd, buf, (size_t)nr);
					if (nw < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
						bypass_close_fd(&client_fd);
						pb->status = DB_BYPASS_STATUS_LISTEN;
					}
				}
			} else if (nr < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
				pb->status = DB_BYPASS_STATUS_FAIL;
				break;
			}
		}
	}

	bypass_close_fd(&client_fd);
	bypass_close_fd(&listen_fd);

	if (pb->chnl == my_chnl && pb->seq == enter_seq) {
		/* 本侧因错误退出：清旁路，避免无人关 */
		if (pb->status == DB_BYPASS_STATUS_FAIL) {
			pb->chnl = 0;
			pb->seq++;
		}
	}
	if (pb->chnl == 0 || pb->chnl != my_chnl)
		pb->status = DB_BYPASS_STATUS_IDLE;

	/* 清串口残留再回 poll */
	if (pChnlPollMgr->com_ctx.fd >= 0)
		pChnlPollMgr->pBaseComm->flush(&pChnlPollMgr->com_ctx);

	fprintf(stderr, "[bypass] chnl %d exit, back to poll\n", (int)my_chnl);
	return 1;
}
