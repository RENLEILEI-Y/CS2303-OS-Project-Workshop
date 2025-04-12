CC = gcc
CFLAGS = -Wall -g
TARGETS = ForkCopy MyCopy PipeCopy shell MergesortMulti MergesortSingle

all: $(TARGETS)

ForkCopy: Copy/ForkCopy.c
	$(CC) $(CFLAGS) -o $@ $<

MyCopy: Copy/MyCopy.c
	$(CC) $(CFLAGS) -o $@ $<

PipeCopy: Copy/PipeCopy.c
	$(CC) $(CFLAGS) -o $@ $<

shell: Shell/shell.c
	$(CC) $(CFLAGS) -o $@ $<

MergesortMulti: Sort/MergesortMulti.c
	$(CC) $(CFLAGS) -o $@ $<

MergesortSingle: Sort/MergesortSingle.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f $(TARGETS)