TARGET = txview.x

CROSS = m68k-xelf-
CC = $(CROSS)gcc
AS = $(CROSS)as
LD = $(CROSS)gcc

CFLAGS = -m68000 -O2
LDFLAGS =

all: $(TARGET)

$(TARGET): himem.o main.o
	$(LD) $(LDFLAGS) -o $@ $^

_build/%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-rm -f *.o *.elf* $(TARGET)
