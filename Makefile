CC= gcc
CFLAGS= -O3 -std=c89

all:
	$(CC) $(CFLAGS) main.c -o main.out -lm -g

retest:
	$(MAKE) clean
	$(MAKE) all
	./main.out mkdir / homie
	./main.out mkdir /homie bin
	./main.out st /homie/bin test main.c



testpt2: retest
	./main.out st /homie/bin test main.out
	./main.out st /homie/bin test Makefile
	./main.out st /homie/bin test main.out
	./main.out gt main2.out /homie/bin/test
	mv main2.out main.out; chmod +x main.out
	./main.out gt main2.out /homie/bin/test
	./main.out gt main2.out /homie/bin/test
	mv main2.out main.out; chmod +x main.out
	./main.out gt main2.out /homie/bin/test
	./main.out gt main2.out /homie/bin/test



clean:
	rm -f *.exe *.out *.o *.dsk
