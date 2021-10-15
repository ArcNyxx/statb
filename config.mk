# see LICENCE file for licensing information
# statb version
VERSION = 0.1.0

# paths
PREFIX = /usr/local

# includes and libs
LIBS = -lX11 -lasound

# flags
STATBCPPFLAGS = -DVERSION=\"$(VERSION)\" -D_XOPEN_SOURCE=600 -w
STATBCFLAGS = $(INCS) $(STATBCPPFLAGS) $(CPPFLAGS) $(CFLAGS)
STATBLDFLAGS = $(LIBS) $(LDFLAGS)
