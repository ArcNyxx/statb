# statb - simple info bar
# Copyright (C) 2021 FearlessDoggo21
# see LICENCE file for licensing information

.POSIX:

include config.mk

SRC = statb.c
OBJ = $(SRC:.c=.o)

all: statb

config.h:
	cp config.def.h config.h

$(OBJ): config.h config.mk

.c.o:
	$(CC) $(CFLAGS) -c $<

statb: $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@

clean:
	rm -rf statb $(OBJ) statb-$(VERSION).tar.gz

dist: clean
	mkdir -p dwm-$(VERSION)
	cp -R LICENCE README Makefile config.mk $(SRC) config.def.h dwm-$(VERSION)
	tar -cf dwm-$(VERSION).tar dwm-$(VERSION)
	gzip dwm-$(VERSION).tar
	rm -rf dwm-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f statb $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/statb

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/statb

options:
	@echo statb build options
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

# implementation defined
.PHONY: all clean dist install uninstall options
