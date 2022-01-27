# statb - simple info bar
# Copyright (C) 2021-2022 FearlessDoggo21
# see LICENCE file for licensing information

.POSIX:

include config.mk

SRC = statb.c util.c
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
	mkdir -p statb-$(VERSION)
	cp -R LICENCE README Makefile config.mk statb.1 $(SRC) config.def.h \
		statb-$(VERSION)
	tar -cf statb-$(VERSION).tar statb-$(VERSION)
	gzip statb-$(VERSION).tar
	rm -rf statb-$(VERSION)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin $(DESTDIR)$(MANPREFIX)/man1
	cp -f statb $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/statb
	sed "s/VERSION/$(VERSION)/g" < statb.1 > $(DESTDIR)$(MANPREFIX)/man1/statb.1
	chmod 644 $(DESTDIR)$(MANPREFIX)/man1/statb.1

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/statb $(DESTDIR)$(MANPREFIX)/man1/statb.1

options:
	@echo statb build options
	@echo "CFLAGS = $(CFLAGS)"
	@echo "LDFLAGS = $(LDFLAGS)"

# implementation defined
.PHONY: all clean dist install uninstall options
