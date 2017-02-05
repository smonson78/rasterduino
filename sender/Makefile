CC=gcc
CFLAGS=-Wall -g
LDFLAGS=
LDLIBS=-lserialport -lfreeimage

TARGET=raster

$(TARGET): main.o
	$(CC) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@
