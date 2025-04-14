CFLAGS=-Wall -Wextra -pedantic -O2 -std=c99 -g

.PHONY: all default clean

all default: smaz

smaz: tool.o smaz.o

clean:
	rm -rf smaz *.o *.exe
