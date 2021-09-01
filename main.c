#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

#define BAT "BAT1"
#define BAT_PATH "/sys/class/power_supply/" BAT

char buf[256], *ptr;
int batCap, batStat, memStat;
const char *batCapPath = BAT_PATH "/capacity";
const char *batStatPath = BAT_PATH "/status";
const char *wifiCard = "wlp0s20f3";
const char *card = "default", *mixer = "Master";
unsigned char internetLen;
char internetBuf[23] = " [IP: ";

#include "func.h"

static volatile sig_atomic_t running = 1;
static Display *dpy;

void term(const int sig) {
    (void)sig;
    running = 0;
}

void leave(void) {
    XStoreName(dpy, DefaultRootWindow(dpy), NULL);
    XFlush(dpy);
    XCloseDisplay(dpy);
}

void pop(void) {
    audio();
    memory();
    memcpy(ptr, internetBuf, internetLen);
    ptr += internetLen;
    battery();
    datetime();
    memcpy(ptr, " doggo-dwm ", 12);
}

int main(void) {
    struct sigaction act = { 0 };
    act.sa_handler = term;
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    act.sa_flags |= SA_RESTART;
    sigaction(SIGUSR1, &act, NULL);

    if (internet()) {
        strcpy(internetBuf, " [IP: Unavailable]");
    }

    batCap = open(batCapPath, O_RDONLY);
    batStat = open(batStatPath, O_RDONLY);
    memStat = open("/proc/meminfo", O_RDONLY);
    if (batCap == -1 || batStat == -1 || memStat == -1) {
        write(STDERR_FILENO, "Unable to open stat files.\n", 27);
        return 1;
    }

    if (!(dpy = XOpenDisplay(0))) {
        write(STDERR_FILENO, "Unable to open display.\n", 24);
        return 1;
    }
    atexit(leave);

    struct timespec start, curr;
    do {
        ptr = (char *)buf;
        if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
            write(STDERR_FILENO, "Unable to get time.\n", 20);
            return 1;
        }

        pop();
        if (XStoreName(dpy, DefaultRootWindow(dpy), buf) < 0) {
            write(STDERR_FILENO, "Unable to store name.\n", 22);
            return 1;
        }
        XFlush(dpy);
        
        if (clock_gettime(CLOCK_MONOTONIC, &curr) == -1) {
            write(STDERR_FILENO, "Unable to get time.\n", 20);
            return 1;
        }
        curr.tv_sec = 0;
        curr.tv_nsec = curr.tv_nsec - start.tv_nsec +
                ((curr.tv_nsec < start.tv_nsec) * 1E9);
        curr.tv_nsec = 5E8 - curr.tv_nsec +
                ((5E8 < curr.tv_nsec) * 1E9);
        if (nanosleep(&curr, 0) == -1 && errno != EINTR) {
            write(STDERR_FILENO, "Unable to sleep.\n", 17);
            return 1;
        }
    } while (running);
}
