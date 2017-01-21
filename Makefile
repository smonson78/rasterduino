MCPU=atmega328p
PROGTYPE=arduino
PROGPORT=/dev/ttyUSB0
PROG_FLAGS=-b 57600
LFUSE=0xE6
HFUSE=0xDF
AVRDUDE=avrdude -p $(MCPU) -c $(PROGTYPE) -P $(PROGPORT) $(PROG_FLAGS)

CC=avr-gcc
CFLAGS=-g -Wall -Os -mmcu=$(MCPU) -DF_CPU=16000000
#LDLIBS=-lgcc

TARGET=raster

$(TARGET).hex: $(TARGET).elf
	avr-objcopy -j .text -j .data -O ihex $^ $@

$(TARGET).elf: main.o longnum.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(LDLIBS)

flash: $(TARGET).hex
	$(AVRDUDE) -U flash:w:$^:i

fuses:
	$(AVRDUDE) -U lfuse:w:$(LFUSE):m
	$(AVRDUDE) -U hfuse:w:$(HFUSE):m

clean:
	$(RM) *.o *.elf *.hex
