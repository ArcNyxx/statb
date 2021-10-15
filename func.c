#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "func.h"
#include "config.h"

extern unsigned int batcap_fd, batstat_fd, memstat_fd;

static inline int
itoa(char *const buf, unsigned int num)
{
	int i = 0, j = 0;
	char tmpbuf[3] = "0";
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

char *
audio(char *const buf)
{
	snd_mixer_t *handle;
	if (
		snd_mixer_open(&handle, 0) < 0 ||
		snd_mixer_attach(handle, audio_card) < 0 ||
		snd_mixer_selem_register(handle, NULL, NULL) < 0 ||
		snd_mixer_load(handle) < 0
	) {
		ERR("statb: unable to load audio mixer\n");
		return NULL;
	}

	snd_mixer_selem_id_t *snd;
	snd_mixer_selem_id_malloc(&snd);
	snd_mixer_selem_id_set_index(snd, 0);
	snd_mixer_selem_id_set_name(snd, audio_mixer);
	snd_mixer_elem_t *elem;
	if ((elem = snd_mixer_find_selem(handle, snd)) == NULL) {
		ERR("statb: unable to load audio mixer\n");
		return NULL;
	}

	long low, high, vol;
	snd_mixer_selem_get_playback_volume_range(elem, &low, &high);
	snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_UNKNOWN, &vol);
	snd_mixer_close(handle);

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
	char tmpbuf[3], stat;
	int len;
	if (
		lseek(batcap_fd, 0, SEEK_SET) == -1 ||
		lseek(batstat_fd, 0, SEEK_SET) == -1 ||
		(len = read(batcap_fd, tmpbuf, 3)) == -1 ||
		read(batstat_fd, &stat, 1) != 1
	) {
		ERR("statb: unable to read battery info\n");
		return NULL;
	}

	buf[0] = bat_stat_char(stat);
	memcpy(buf + 1, tmpbuf, len);
	buf[len + 1] = '%';
	return buf + len + 2;
}

char *
internet(char *const buf)
{
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1) {
		ERR("statb: unable to get ip addresses\n");
	}

	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL) {
			continue;
		}

		if (!strcmp(ifa->ifa_name, wifi_card) &&
			ifa->ifa_addr->sa_family == AF_INET) {
			if (inet_ntop(AF_INET, ifa->ifa_addr->sa_data + 2, buf, 16) == NULL) {
				ERR("statb: unable to convert ip to text\n");
				return NULL;
			}
			freeifaddrs(ifaddr);

			return buf + strlen(buf);
		}
	}
	freeifaddrs(ifaddr);

	memcpy(buf, internet_unavail, sizeof(internet_unavail));
	return buf + sizeof(internet_unavail);
}

char *
datetime(char *const buf)
{
	time_t timep = time(NULL);
	struct tm *tm = localtime(&timep);

	if (tm->tm_mday / 10 != 1) {
		int date = tm->tm_mday % 10;
		char first, second = 'd' + ('t' & -(date != 1));
		if (date == 1) {
			first = 's';
		} else if (date == 2) {
			first = 'n';
		} else {
			first = 'r';
		}

		date_fmt[ORDIND_DATE] = first;
		date_fmt[ORDIND_DATE + 1] = second;
	}

	return buf + strftime(buf, 128, date_fmt, tm);
}
