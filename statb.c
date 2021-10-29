/*
 * statb - simple info bar
 * Copyright (C) 2021 FearlessDoggo21
 * see LICENCE file for licensing information
 */

#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define DOTOKEN(x, y) x ## y
#define TOKEN(x, y) DOTOKEN(x, y)
#define SELEM(x) TOKEN(snd_mixer_selem_, x)

typedef struct func Func;

struct func {
	char *(*func)(char *const);
	const char *iden;
	size_t iden_len;
};

void term(const int sig);
int itoa(char *const buf, unsigned int num);
static inline char bat_stat_char(char c);

char *audio(char *const buf);
char *memory(char *const buf);
char *battery(char *const buf);
char *internet(char *const buf);
char *datetime(char *const buf);

static volatile sig_atomic_t running = 1;
int batcap_fd, batstat_fd, memstat_fd;

#include "config.h"

void
term(const int sig)
{
	(void)sig;
	running = 0;
}

int
itoa(char *const buf, unsigned int num)
{
	int i = 0, j = 0;
	char tmpbuf[3];
	do {
		tmpbuf[i] = '0' + (num % 10);
		num /= 10;
		++i;
	} while(num);

	while (j != i) {
		buf[j] = tmpbuf[i - j - 1];
		++j;
	}
	return i;
}

static inline char
bat_stat_char(char c)
{
        switch (c) {
        case 'F':
                return 'f';
        case 'C':
                return '+';
        case 'D':
                return '-';
        case 'U':
                return 'x';
        default:
                return 0;
        }
}

char *
audio(char *const buf)
{
	snd_mixer_t *mixer;
	if (
		snd_mixer_open(&mixer, 0) ||
		snd_mixer_attach(mixer, audio_card) ||
		SELEM(register(mixer, NULL, NULL)) ||
		snd_mixer_load(mixer)
	) {
		ERR("statb: unable to load audio mixer\n");
		return NULL;
	}

	snd_mixer_selem_id_t *master;
	if (SELEM(id_malloc(&master))) {
		ERR("statb: unable to allocate memory\n");
		return NULL;
	}
	SELEM(id_set_index(master, 0));
	SELEM(id_set_name(master, audio_mixer));

	snd_mixer_elem_t *elem;
	if ((elem = snd_mixer_find_selem(mixer, master)) == NULL) {
		ERR("statb: unable to load audio mixers\n");
		memcpy(buf, audio_error, sizeof(audio_error) - 1);
		return buf + sizeof(audio_error) - 1;
	}

	int audio_switch;
	SELEM(get_playback_switch(elem, SND_MIXER_SCHN_UNKNOWN, &audio_switch));
	if (!audio_switch) {
		memcpy(buf, audio_mute, sizeof(audio_mute) - 1);
		return buf + sizeof(audio_mute) - 1;
	}

	long low, high, vol;
	SELEM(get_playback_volume_range(elem, &low, &high));
	SELEM(get_playback_volume(elem, SND_MIXER_SCHN_UNKNOWN, &vol));
	free(master);
	snd_mixer_close(mixer);

	int len = itoa(buf, ((float)vol / (float)high) * 100);
	buf[len] = '%';
	return buf + len + 1;
}

char *
memory(char *const buf)
{
	static char tmpbuf[52];
	if (lseek(memstat_fd, 0, SEEK_SET) == -1 ||
		read(memstat_fd, tmpbuf, 52) != 52) {
		ERR("statb: unable to read memory info\n");
		return NULL;
	}

	char *endptr;
	float total = strtol(tmpbuf + 10, &endptr, 10);
	float free = strtol(endptr + 13, NULL, 10);

	int len = itoa(buf, ((total - free) / total) * 100);
	buf[len] = '%';
	return buf + len + 1;
}

char *
battery(char *const buf)
{
	ssize_t len;
	char stat;
	if (
		lseek(batcap_fd, 0, SEEK_SET) == -1 ||
		lseek(batstat_fd, 0, SEEK_SET) == -1 ||
		(len = read(batcap_fd, buf + 1, 3)) == -1 ||
		read(batstat_fd, &stat, 1) != 1
	) {
		ERR("statb: unable to read battery info\n");
		return NULL;
	}

	buf[0] = bat_stat_char(stat);
	len -= (buf[2] == '\n' || buf[3] == '\n');
	buf[len + 1] = '%';
	return buf + len + 2;
}

char *
internet(char *const buf)
{
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1) {
		perror("");
		ERR("statb: unable to get ip addresses\n");
		ifaddr = NULL;
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		if (!strcmp(ifa->ifa_name, wifi_card)) {
#ifndef INTERNET_NOSHOW_IP
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;
			if (inet_ntop(AF_INET, ifa->ifa_addr->sa_data + 2, buf, 16) == NULL) {
				ERR("statb: unable to convert ip to text\n");
				return NULL;
			}
			freeifaddrs(ifaddr);
			return buf + strlen(buf);
#else
			freeifaddrs(ifaddr);
			memcpy(buf, internet_connect, sizeof(internet_connect) - 1);
			return buf + sizeof(internet_connect) - 1;
#endif
		}
	}
	if (ifaddr != NULL)
		freeifaddrs(ifaddr);
	memcpy(buf, internet_unavail, sizeof(internet_unavail) - 1);
	return buf + sizeof(internet_unavail) - 1;
}

char *
datetime(char *const buf)
{
	time_t timep = time(NULL);
	struct tm *tm = localtime(&timep);

	if (tm->tm_mday / 10 != 1) {
		int date = tm->tm_mday % 10;
		if (date <= 3 && date) {
			char first = 'n', second = 'd';

			if (date == 1)
				first = 's', second = 't';
			else if (date == 3)
				first = 'r';

			date_fmt[ORDIND_DATE] = first;
			date_fmt[ORDIND_DATE + 1] = second;
		}
	}
	return buf + strftime(buf, 128, date_fmt, tm);
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
	char buf[BUF_LEN], *ptr = buf;
	do {
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
			ERR("statb: unable to get time\n");
			goto exit;
		}

		size_t i;
		for (i = 0; i < sizeof(funcs) / sizeof(struct func); ++i) {
			memcpy(ptr, sep_open, sizeof(sep_open) - 1);
			memcpy(ptr + sizeof(sep_open) - 1, funcs[i].iden, funcs[i].iden_len);

			if ((ptr = funcs[i].func(ptr + sizeof(sep_open) +
				funcs[i].iden_len - 1)) == NULL) {
				goto exit;
			}

			memcpy(ptr, sep_close, sizeof(sep_close) - 1);
			ptr += sizeof(sep_close) - 1;
		}
		memcpy(ptr, name, sizeof(name));

		ptr = buf;
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
