# statb - simple info bar
# Copyright (C) 2021 FearlessDoggo21
# see LICENCE file for licensing information

# statb version
VERSION = 0.1.1

# paths
PREFIX = /usr/local

# includes and libs
LIBS = -lX11 -lasound

# flags
WPROFILE = -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes \
-Wmissing-declarations -Wswitch-default -Wunreachable-code -Wcast-align \
-Wpointer-arith -Wbad-function-cast -Winline -Wundef -Wnested-externs \
-Wcast-qual -Wshadow -Wwrite-strings -Wno-unused-parameter -Wfloat-equal
STATBCPPFLAGS = -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=600 $(WPROFILE) \
	-std=c99 -pedantic -O2
STATBCFLAGS = $(STATBCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
STATBLDFLAGS = $(LIBS) $(LDFLAGS)
