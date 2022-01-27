/* statb - simple info bar
 * Copyright (C) 2021 FearlessDoggo21
 * see LICENCE file for licensing information */

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

#include "util.h"

typedef struct func {
	char *(*func)(char *);
	const char *iden;
	size_t iden_len;
} Func;

static void term(const int sig);

static char *audio(char *buf);
static char *memory(char *buf);
static char *battery(char *buf);
static char *internet(char *buf);
static char *datetime(char *buf);

static volatile sig_atomic_t running = 1;
static int batcap_fd, batstat_fd, memstat_fd;
Display *dpy;

#include "config.h"

static void
term(const int sig)
{
	(void)sig;
	running = 0;
}

static char *
audio(char *buf)
{
	snd_mixer_t *mixer;
	if (
		snd_mixer_open(&mixer, 0) != 0 ||
		snd_mixer_attach(mixer, audio_card) != 0 ||
		snd_mixer_selem_register(mixer, NULL, NULL) != 0 ||
		snd_mixer_load(mixer) != 0
	)
		die("statb: unable to load audio mixer\n");

	snd_mixer_selem_id_t *master;
	if (snd_mixer_selem_id_malloc(&master))
		die("statb: unable to allocate memory");
	snd_mixer_selem_id_set_index(master, 0);
	snd_mixer_selem_id_set_name(master, audio_mixer);

	snd_mixer_elem_t *elem;
	if ((elem = snd_mixer_find_selem(mixer, master)) == NULL)
		die("statb: unable to load audio mixers\n");

	int mute;
	long low, high, vol;

	snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_UNKNOWN, &mute);
	snd_mixer_selem_get_playback_volume_range(elem, &low, &high);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_UNKNOWN, &vol);

	free(master);
	snd_mixer_close(mixer);

	buf[0] = mute ? 'u' : 'm';
	size_t len = ultostr(((float)vol / (float)high) * 100, buf + 1);
	buf[len + 1] = '%';
	return buf + len + 2;
}

static char *
memory(char *buf)
{
	static char tmpbuf[52];
	lseek(memstat_fd, 0, SEEK_SET);
	if (read(memstat_fd, tmpbuf, 52) != 52)
		die("statb: unable to read memory info");

	char *endptr;
	float total = strtol(tmpbuf + 10, &endptr, 10);
	float free = strtol(endptr + 13, NULL, 10);

	size_t len = ultostr(((total - free) / total) * 100, buf);
	buf[len] = '%';
	return buf + len + 1;
}

static char *
battery(char *buf)
{
	size_t len;
	lseek(batcap_fd, 0, SEEK_SET);
	lseek(batstat_fd, 0, SEEK_SET);
	if ((len = read(batcap_fd, buf + 1, 3)) == (size_t)-1 ||
			read(batstat_fd, &buf[0], 1) != 1)
		die("statb: unable to read files");

	static const struct {
		char read, new;
	} conv[] = { { 'F', 'f' }, { 'C', '+' }, { 'D', '-' }, { 'U', 'x' } };
	for (size_t i = 0; i < 4; ++i)
		if (buf[0] == conv[i].read) {
			buf[0] = conv[i].new;
			break;
		}
	
	len -= (buf[2] == '\n' || buf[3] == '\n');
	buf[len + 1] = '%';
	return buf + len + 2;
}

static char *
internet(char *buf)
{
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1)
		die("statb: unable to get ip addresses");

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (!strcmp(ifa->ifa_name, wifi_card)) {
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;

#ifdef INTERNET_NOSHOW_IP
			freeifaddrs(ifaddr);
			memcpy(buf, internet_connect, sizeof(internet_connect) - 1);
			return buf + sizeof(internet_connect) - 1;
#else
			if (inet_ntop(AF_INET, ifa->ifa_addr->sa_data + 2, buf, 16) == NULL)
				die("statb: unable to convert ip to text");
			freeifaddrs(ifaddr);
			return buf + strlen(buf);
#endif /* INTERNET_NOSHOW_IP */
		}
	}
	freeifaddrs(ifaddr);
	memcpy(buf, internet_unavail, sizeof(internet_unavail) - 1);
	return buf + sizeof(internet_unavail) - 1;
}

static char *
datetime(char *buf)
{
#define DATE_CASE(num, first, second) \
case num: \
	date_fmt[ORDIND_DATE] = first; \
	date_fmt[ORDIND_DATE + 1] = second; \
	break;

	/* get human interpretable time */
	time_t timep = time(NULL);
	struct tm *timeh = localtime(&timep);

	/* get ordinal numbered date correctly */
	if (timeh->tm_mday / 10 != 1)
		switch (timeh->tm_mday % 10) {
		DATE_CASE(1, 's', 't')
		DATE_CASE(2, 'n', 'd')
		DATE_CASE(3, 'r', 'd')
		default:
			date_fmt[ORDIND_DATE] = 't';
			date_fmt[ORDIND_DATE + 1] = 'h';
			break;
		}
	return buf + strftime(buf, 128, date_fmt, timeh);
#undef DATE_CASE
}

int
main(void)
{
	if (
		(batcap_fd = open(batcap_path, O_RDONLY)) == -1 ||
		(batstat_fd = open(batstat_path, O_RDONLY)) == -1 ||
		(memstat_fd = open(mem_path, O_RDONLY)) == -1
	)
		die("statb: unable to create file descriptors");
	
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		die("statb: unable to open root display window\n");

	struct sigaction act = { 0 };
	act.sa_handler = term;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);

	struct timespec start, cur;
	char buf[BUF_LEN], *ptr = buf;
	do {
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
			die("statb: unable to get time");

		size_t i;
		for (i = 0; i < sizeof(funcs) / sizeof(Func); ++i) {
			memcpy(ptr, sep_open, sizeof(sep_open) - 1);
			memcpy(ptr + sizeof(sep_open) - 1, funcs[i].iden, funcs[i].iden_len);

			ptr = funcs[i].func(ptr + sizeof(sep_open) + funcs[i].iden_len - 1);

			memcpy(ptr, sep_close, sizeof(sep_close) - 1);
			ptr += sizeof(sep_close) - 1;
		}
		memcpy(ptr, name, sizeof(name));

		ptr = buf;
		if (XStoreName(dpy, DefaultRootWindow(dpy), ptr) < 0)
			die("statb: unable to store name\n");
		XFlush(dpy);

		/* messy maths to ensure each cycle is exactly 0.5 seconds */
		if (clock_gettime(CLOCK_MONOTONIC, &cur) == -1)
			die("statb: unable to get time");
		cur.tv_sec = 0;
		cur.tv_nsec = cur.tv_nsec - start.tv_nsec +
			((cur.tv_nsec < start.tv_nsec) * 1E9);
		cur.tv_nsec = 5E8 - cur.tv_nsec + ((5E8 < cur.tv_nsec) * 1E9);
		if (nanosleep(&cur, NULL) == -1 && errno != EINTR)
			die("statb: unable to sleep");
	} while (running);

	XStoreName(dpy, DefaultRootWindow(dpy), NULL);
        XFlush(dpy);
        XCloseDisplay(dpy);
}
