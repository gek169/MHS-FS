CC= gcc
CFLAGS= -O3 -std=c89

all:
	$(CC) $(CFLAGS) main.c -o main.out -lm -g

clean:
	rm -f *.exe *.out *.o *.dsk
