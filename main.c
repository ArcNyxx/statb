#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "func.h"

static void term(const int sig);

static volatile sig_atomic_t running = 1;
unsigned int batcap_fd, batstat_fd, memstat_fd;

#include "config.h"

static void
term(const int sig)
{
	(void)sig;
	running = 0;
}

int
main(void)
{
	struct sigaction act = { 0 };
	act.sa_handler = term;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	batcap_fd = open(BAT_PATH BAT BAT_PATH_CAP, O_RDONLY);
	batstat_fd = open(BAT_PATH BAT BAT_PATH_STAT, O_RDONLY);
	memstat_fd = open(mem_path, O_RDONLY);
	if (batcap_fd == -1 || batstat_fd == -1 || memstat_fd == -1) {
		ERR("statb: unable to create file descriptors\n");
		return 1;
	}

	Display *dpy;
	if ((dpy = XOpenDisplay(NULL)) == NULL) {
		ERR("statb: unable to open root display window\n");
		return 1;
	}

	struct timespec start, cur;
	char buf[256], *ptr = (char *)buf;
	do {
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
			ERR("statb: unable to get time\n");
			goto exit;
		}

		int i;
		for (i = 0; i < sizeof(funcs) / sizeof(struct func); ++i) {
			memcpy(ptr, sep_open, sizeof(sep_open) - 1);
			memcpy(ptr + sizeof(sep_open) - 1, funcs[i].iden, funcs[i].iden_len);

			if ((ptr = funcs[i].func(ptr + sizeof(sep_open) +
				funcs[i].iden_len - 1)) == NULL) {
				ERR("statb: unable to perform function\n");
				goto exit;
			}

			memcpy(ptr, sep_close, sizeof(sep_close) - 1);
			ptr += sizeof(sep_close) - 1;
		}
		memcpy(ptr, name, sizeof(name) - 1);

		ptr = (char *)buf;
		if (XStoreName(dpy, DefaultRootWindow(dpy), ptr) < 0) {
			ERR("statb: unable to store name\n");
			goto exit;
		}
		XFlush(dpy);

		if (clock_gettime(CLOCK_MONOTONIC, &cur) == -1) {
			ERR("statb: unable to get time\n");
			goto exit;
		}
		cur.tv_sec = 0;
		cur.tv_nsec = cur.tv_nsec - start.tv_nsec +
			((cur.tv_nsec < start.tv_nsec) * 1E9);
		cur.tv_nsec = 5E8 - cur.tv_nsec + ((5E8 < cur.tv_nsec) * 1E9);
		if (nanosleep(&cur, NULL) == -1 && errno != EINTR) {
			ERR("statb: unable to sleep\n");
			goto exit;
		}
	} while (running);

exit:
	XStoreName(dpy, DefaultRootWindow(dpy), NULL);
	XFlush(dpy);
	XCloseDisplay(dpy);
	return running;
}
