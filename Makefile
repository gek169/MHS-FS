CC= cc
CFLAGS= -O3 -std=c89

all:
	$(CC) $(CFLAGS) main.c vhd.c -o main.out -lm -g

test3: clean all
	./main.out mkdir / bruhfolder
	./main.out mkdir /bruhfolder otherfolder
	#this should put a hole in the filesystem, causing a sort of fragmentation.

	./main.out st /bruhfolder/otherfolder main.c main.c
	./main.out st /bruhfolder/otherfolder vhd.c vhd.c
	./main.out st /bruhfolder/otherfolder MHS.h MHS.h
	./main.out st /bruhfolder/otherfolder MHS2.h MHS.h
	./main.out st /bruhfolder/otherfolder MHS3.h MHS.h
	./main.out st /bruhfolder/otherfolder LICENSE LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE2 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE3 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE4 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE4 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE4 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE5 LICENSE
	./main.out st /bruhfolder/otherfolder LICENSE6 LICENSE
	./main.out st /bruhfolder/otherfolder main.out main.out
	./main.out st /bruhfolder/otherfolder main2.out main.out
	./main.out st /bruhfolder/otherfolder main3.out main.out
	./main.out st /bruhfolder/otherfolder testing_realloc main.c
	./main.out st /bruhfolder/otherfolder testing_realloc main.out
	./main.out st /bruhfolder/ main4.out main.out
	./main.out st /bruhfolder/ main5.out main.out
	./main.out st /bruhfolder/ main6.out main.out
	./main.out st /bruhfolder/ main7.out main.out
	./main.out st /bruhfolder/ main8.out main.out
	./main.out st /bruhfolder/ main9.out main.out
	./main.out st /bruhfolder/ main10.out main.out
	./main.out st /bruhfolder/ main11.out main.out
	./main.out st /bruhfolder/ main12.out main.out
	./main.out st /bruhfolder/ main13.out main.out
	./main.out st /bruhfolder/ main14.out main.out
	./main.out st /bruhfolder/ main15.out main.out
	./main.out st /bruhfolder/ main16.out main.out
	./main.out st /bruhfolder/ main17.out main.out
	./main.out st /bruhfolder/ main18.out main.out
	./main.out st /bruhfolder/ main19.out main.out
	./main.out st /bruhfolder/ main20.out main.out


retest: clean all
	./main.out mkdir / homie
	./main.out mkdir /homie bin
	./main.out st /homie/bin test main.c



testpt2: retest
	./main.out st /homie/bin test main.out
	./main.out st /homie/bin test Makefile
	./main.out st /homie/bin test main.out
	./main.out st /homie/bin test2 Makefile
	./main.out st /homie/bin test3 Makefile
	./main.out st /homie/bin test4 main.out
	./main.out st /homie/bin test5 main.out
	./main.out rm /homie/bin test5 
	./main.out rm /homie/bin test4 
	./main.out rm /homie/bin test3 
	./main.out rm /homie/bin test2  
	./main.out st /homie/bin test2 Makefile
	./main.out st /homie/bin test3 Makefile
	./main.out st /homie/bin test4 main.out
	./main.out st /homie/bin test5 main.out
	./main.out st /homie/bin test6 main.out
	./main.out st /homie/ main.c main.c
	./main.out st /homie/ vhd.c vhd.c
	./main.out st /homie test7 main.out
	./main.out st / test main.out
	./main.out gt main2.out /homie/bin/test
	#mv main2.out main.out; chmod +x main.out
	./main.out gt vhd.c.out /homie/vhd.c
	./main.out gt main.c.out /homie/main.c
	./main.out gt main2.out /homie/bin/test
	#mv main2.out main.out; chmod +x main.out
	./main.out gt main2.out /homie/bin/test
	./main.out gt main2.out /homie/bin/test



clean:
	rm -f *.exe *.out *.o *.dsk
