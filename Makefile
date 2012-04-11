CC = msp430-gcc
CFLAGS = -Os -Wall -g

all: download

%.hex: %.elf
	msp430-objcopy -O ihex $< $@

%.elf: %.c
	msp430-gcc $(CFLAGS) -mmcu=msp430x2012 -o $@ $<

clean:
	rm -f main.hex main.elf

download: main.hex
	mspdebug rf2500 "prog main.hex"
