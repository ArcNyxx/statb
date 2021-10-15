# statb - simple info bar for dwm
# See LICENSE file for copyright and license details.
.POSIX:

include config.mk

SRC = statb.c
OBJ = $(SRC:.c=.o)

CC = gcc --std=c99

all: options statb

options:
	@echo statb build options:
	@echo "CFLAGS  = $(STATBCFLAGS)"
	@echo "LDFLAGS = $(STATBLDFLAGS)"
	@echo "CC      = $(CC)"

# note for development purposes overwrite
config.h: config.def.h
	cp config.def.h config.h

.c.o:
	$(CC) $(STATBCFLAGS) -c $<

main.o: config.h
func.o: config.h

$(OBJ): config.h config.mk

statb: $(OBJ)
	$(CC) -o $@ $(OBJ) $(STATBLDFLAGS)

clean:
	rm -f st $(OBJ) statb-$(VERSION).tar.gz

dist: clean
	mkdir -p statb-$(VERSION)
	cp -R LICENSE Makefile README config.mk \
		config.def.h $(SRC) statb-$(VERSION)
	tar -cf - statb-$(VERSION) | gzip > statb-$(VERSION).tar.gz
	rm -rf statb-$(VERSION)

install: statb
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp -f statb $(DESTDIR)$(PREFIX)/bin
	chmod 755 $(DESTDIR)$(PREFIX)/bin/statb

uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/statb

.PHONY: all options clean dist install uninstall
