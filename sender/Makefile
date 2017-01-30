CC=gcc
CFLAGS=-Wall -g
LDFLAGS=
LDLIBS=-lserialport

TARGET=raster

$(TARGET): main.o
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
