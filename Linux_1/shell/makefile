xsh : bison.tab.o execute.o init.o handleInBuff.o search.o handleJob.o handleSignal.o
	gcc -o  xsh bison.tab.o execute.o init.o handleInBuff.o search.o handleJob.o handleSignal.o
bison.tab.o : bison.tab.c global.h
	gcc -c bison.tab.c -g bison.tab.c
execute.o : execute.c global.h handleJob.o handleSignal.o
	gcc -c execute.c -g execute.c
init.o : init.c global.h handleSignal.o
	gcc -c init.c -g init.c
handleInBuff.o : handleInBuff.c global.h search.o
	gcc -c handleInBuff.c -g handleInBuff.c
search.o : search.c global.h
	gcc -c search.c -g search.c
handleJob.o : handleJob.c global.h
	gcc -c handleJob.c -g handleJob.c
handleSignal.o : handleSignal.c global.h
	gcc -c handleSignal.c -g handleSignal.c

clean :
	rm xsh bison.tab.o execute.o init.o handleInBuff.o search.o handleJob.o handleSignal.o
