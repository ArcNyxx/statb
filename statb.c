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

#define SIMPLE_WRITE(string) \
memcpy(buf, string, sizeof(string) - 1); \
return buf + sizeof(string) - 1;
#define DIE(string) die(string, sizeof(string))

typedef struct func {
	char *(*func)(char *const);
	const char *iden;
	size_t iden_len;
} Func;

static void term(const int sig);
static void destroy(void);
static void die(const char *error, const size_t len);

static int itoa(char *const buf, unsigned int num);

static char *audio(char *const buf);
static char *memory(char *const buf);
static char *battery(char *const buf);
static char *internet(char *const buf);
static char *datetime(char *const buf);

static volatile sig_atomic_t running = 1;
static int batcap_fd, batstat_fd, memstat_fd;
static Display *dpy;

#include "config.h"

static void
term(const int sig)
{
	(void)sig;
	running = 0;
}

static void
destroy(void)
{
	XStoreName(dpy, DefaultRootWindow(dpy), NULL);
	XFlush(dpy);
	XCloseDisplay(dpy);
}

static void
die(const char *error, const size_t len)
{
	write(STDERR_FILENO, error, len);
	exit(1);
}

static int
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

static char *
audio(char *const buf)
{
	snd_mixer_t *mixer;
	if (
		snd_mixer_open(&mixer, 0) ||
		snd_mixer_attach(mixer, audio_card) ||
		SELEM(register(mixer, NULL, NULL)) ||
		snd_mixer_load(mixer)
	)
		DIE("statb: unable to load audio mixer\n");

	snd_mixer_selem_id_t *master;
	if (SELEM(id_malloc(&master)))
		DIE("statb: unable to allocate memory\n");
	SELEM(id_set_index(master, 0));
	SELEM(id_set_name(master, audio_mixer));

	snd_mixer_elem_t *elem;
	if ((elem = snd_mixer_find_selem(mixer, master)) == NULL)
		DIE("statb: unable to load audio mixers\n");
	free(master);

	int audio_switch;
	SELEM(get_playback_switch(elem, SND_MIXER_SCHN_UNKNOWN, &audio_switch));
	if (!audio_switch) {
		snd_mixer_close(mixer);
		SIMPLE_WRITE(audio_mute)
	}

	long low, high, vol;
	SELEM(get_playback_volume_range(elem, &low, &high));
	SELEM(get_playback_volume(elem, SND_MIXER_SCHN_UNKNOWN, &vol));
	snd_mixer_close(mixer);

	int len = itoa(buf, ((float)vol / (float)high) * 100);
	buf[len] = '%';
	return buf + len + 1;
}

static char *
memory(char *const buf)
{
	static char tmpbuf[52];
	if (
		lseek(memstat_fd, 0, SEEK_SET) == -1 ||
		read(memstat_fd, tmpbuf, 52) != 52
	)
		DIE("statb: unable to read memory info\n");

	char *endptr;
	float total = strtol(tmpbuf + 10, &endptr, 10);
	float free = strtol(endptr + 13, NULL, 10);

	int len = itoa(buf, ((total - free) / total) * 100);
	buf[len] = '%';
	return buf + len + 1;
}

static char *
battery(char *const buf)
{
	ssize_t len;
	char stat;
	if (
		lseek(batcap_fd, 0, SEEK_SET) == -1 ||
		lseek(batstat_fd, 0, SEEK_SET) == -1 ||
		(len = read(batcap_fd, buf + 1, 3)) == -1 ||
		read(batstat_fd, &stat, 1) != 1
	)
		DIE("statb: unable to read battery info\n");

        switch (stat) {
        case 'F':
                buf[0] = 'f';
		break;
        case 'C':
                buf[0] = '+';
		break;
        case 'D':
                buf[0] = '-';
		break;
        case 'U':
                buf[0] = 'x';
		break;
        default:
                return NULL;
        }
	len -= (buf[2] == '\n' || buf[3] == '\n');
	buf[len + 1] = '%';
	return buf + len + 2;
}

static char *
internet(char *const buf)
{
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1)
		DIE("statb: unable to get ip addresses\n");

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		if (!strcmp(ifa->ifa_name, wifi_card)) {
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;

#ifdef INTERNET_NOSHOW_IP
			freeifaddrs(ifaddr);
			SIMPLE_WRITE(internet_connect)
#else
			if (inet_ntop(AF_INET, ifa->ifa_addr->sa_data + 2, buf, 16) == NULL)
				DIE("statb: unable to convert ip to text\n");
			freeifaddrs(ifaddr);
			return buf + strlen(buf);
#endif /* INTERNET_NOSHOW_IP */
		}
	}
	freeifaddrs(ifaddr);
	SIMPLE_WRITE(internet_unavail)
}

static char *
datetime(char *const buf)
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
		DIE("statb: unable to create file descriptors\n");
	
	if ((dpy = XOpenDisplay(NULL)) == NULL)
		DIE("statb: unable to open root display window\n");

	struct sigaction act = { 0 };
	act.sa_handler = term;
	sigaction(SIGINT, &act, NULL);
	sigaction(SIGTERM, &act, NULL);
	atexit(destroy);

	struct timespec start, cur;
	char buf[BUF_LEN], *ptr = buf;
	do {
		if (clock_gettime(CLOCK_MONOTONIC, &start) == -1)
			DIE("statb: unable to get time\n");

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
			DIE("statb: unable to store name\n");
		XFlush(dpy);

		if (clock_gettime(CLOCK_MONOTONIC, &cur) == -1)
			DIE("statb: unable to get time\n");
		cur.tv_sec = 0;
		cur.tv_nsec = cur.tv_nsec - start.tv_nsec +
			((cur.tv_nsec < start.tv_nsec) * 1E9);
		cur.tv_nsec = 5E8 - cur.tv_nsec + ((5E8 < cur.tv_nsec) * 1E9);
		if (nanosleep(&cur, NULL) == -1 && errno != EINTR)
			DIE("statb: unable to sleep\n");
	} while (running);
	exit(0);
}
