CC= gcc
CFLAGS= -O3

all:
	$(CC) $(CFLAGS) main.c -o main.out -lm -g

clean:
	rm -f *.exe *.out *.o *.dsk
