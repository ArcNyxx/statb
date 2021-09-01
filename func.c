#include <alsa/asoundlib.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "func.h"

extern char *ptr;
extern int batCap, batStat, memStat;
extern unsigned char internetLen;
extern char internetBuf[];
extern const char *wifiCard, *card, *mixer;

static inline long perc(float top, float bottom) {
    return (top / bottom) * 100;
}

void audio(void) {
    snd_mixer_t *handle;
    if (
        (snd_mixer_open(&handle, 0) < 0) || 
        (snd_mixer_attach(handle, card) < 0) ||
        (snd_mixer_selem_register(handle, 0, 0) < 0) ||
        (snd_mixer_load(handle) < 0)
    ) {
        write(STDERR_FILENO, "Unable to load sound mixer.\n", 28);
        exit(1);
    }

    snd_mixer_selem_id_t *snd;
    snd_mixer_selem_id_alloca(&snd);
    snd_mixer_selem_id_set_index(snd, 0);
    snd_mixer_selem_id_set_name(snd, mixer);
    snd_mixer_elem_t *elem;
    if (!(elem = snd_mixer_find_selem(handle, snd))) {
        write(STDERR_FILENO, "Unable to load sound mixer.\n", 28);
        exit(1);
    }

    long low, high, vol;
    snd_mixer_selem_get_playback_volume_range(elem, &low, &high);
    snd_mixer_selem_get_playback_volume(elem,
            SND_MIXER_SCHN_UNKNOWN, &vol);
    snd_mixer_close(handle);

    ptr += sprintf(ptr, " [Volume: %ld%%]", perc(vol, high));
}

void memory(void) {
    char buf[55];
    if (
        lseek(memStat, 0, SEEK_SET) == -1 ||
        read(memStat, buf, 55) != 55
    ) {
        write(STDERR_FILENO, "Unable to read memory info.\n", 28);
        exit(1);
    }

    char *endptr;
    float total = strtol(buf + 12, &endptr, 10);
    float free = strtol(endptr + 15, 0, 10);
    ptr += sprintf(ptr, " [Memory: %ld%%]", perc(total - free, total));
}

int internet(void) {
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        return 1;
    }

    for (ifa = ifaddr; ifa != 0; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) {
            continue;
        }

        if (!strcmp(ifa->ifa_name, wifiCard)
                && ifa->ifa_addr->sa_family == AF_INET) {
            if (inet_ntop(AF_INET, ifa->ifa_addr->sa_data + 2,
                    internetBuf + 6, 16) == 0) {
                return 1;
            }

            internetLen = strlen(internetBuf);
            internetBuf[internetLen] = ']';
            ++internetLen;

            freeifaddrs(ifaddr);
            return 0;
        }
    }

    memcpy(internetBuf + 6, "Unavailable]", 12);
    internetLen = 18;

    freeifaddrs(ifaddr);
    return 0;
}

char batteryStatus(const char ch) {
    switch (ch) {
    case 'F':
        return 'f';
    case 'C':
        return '+';
    case 'D':
        return '-';
    default: // case 'U':
        return 'x';
    }
}

void battery(void) {
    char buf[4], stat;
    int len;
    if (
        lseek(batCap, 0, SEEK_SET) == -1 ||
        lseek(batStat, 0, SEEK_SET) == -1 ||
        (len = read(batCap, buf, 4)) == -1 ||
        read(batStat, &stat, 1) != 1
    ) {
        write(STDERR_FILENO, "Unable to read battery info.\n", 29);
        exit(1);
    }

    buf[len - 1] = 0;
    ptr += sprintf(ptr, " [Battery: %c%s%%]", batteryStatus(stat), buf);
}

void datetime(void) {
    time_t timep = time(0);
    struct tm *tm = localtime(&timep);

    char fmt[26] = " [%a %b %dth %H:%M:%S %Z]";
    if (tm->tm_mday / 10 != 1) {
        switch (tm->tm_mday % 10) {
        case 1:
            fmt[10] = 's';
            fmt[11] = 't';
            break;
        case 2:
            fmt[10] = 'n';
            fmt[11] = 'd';
            break;
        case 3:
            fmt[10] = 'r';
            fmt[11] = 'd';
            break;
        }
    }

    ptr += strftime(ptr, 256, fmt, tm);
}
