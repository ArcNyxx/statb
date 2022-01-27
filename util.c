/* statb - simple info bar
 * Copyright (C) 2022 FearlessDoggo21
 * see LICENCE file for licensing information */

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>

#include "util.h"

extern Display *dpy;

/* perror based on final fmt char being '\n' */
void
die(const char *fmt, ...)
{
	if (fmt[strlen(fmt) - 1] == '\n') {
		va_list ap;
		va_start(ap, fmt);
		vfprintf(stderr, fmt, ap);
		va_end(ap);
	} else {
		perror(fmt);
	}

	XStoreName(dpy, DefaultRootWindow(dpy), NULL);
	XFlush(dpy);
	XCloseDisplay(dpy);

	exit(1);
}
