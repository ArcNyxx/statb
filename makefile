SRC = func.c main.c
OBJ = $(SRC:.c=.o)

CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lX11 -lasound

status: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o:
	$(CC) -c $(CFLAGS) $<

clean:
	rm -f $(OBJ) status

install: status
	cp -f status /usr/bin/status
	
uninstall:
	rm -f /usr/bin/status
