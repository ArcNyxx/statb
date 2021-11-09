# statb - simple info bar
statb is a simple info bar for window managers which output WM_NAME as an info
bar.

## Requirements
Edit config.mk to match your local setup

Afterwards enter the following command to build and install statb (if
necessary as root):
```
make clean install
```

## Running statb
Add the following line the your .xinitrc in the middle to start statb using
startx:
```
statb &
```

## Configuration
Configuration of statb is done by creating a custom config.h and
recompiling the source code.
Run `make clean install` again to rebuild and install statb.