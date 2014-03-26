project : build clean

build : job enq deq stat

job : job.o error.o
	cc -o job job.o error.o
job.o : 
	cc -c job.c
error.o :
	cc -c error.c

enq : enq.o error.o
	cc -o enq enq.o error.o
enq.o : 
	cc -c enq.c error.c job.h

stat : stat.o error.o
	cc -o stat stat.o error.o
stat.o : 
	cc -c stat.c job.h error.c

deq : deq.o error.o
	cc -o deq deq.o error.o
deq.o :
	cc -c deq.c job.h error.c

clean :
	rm job.o error.o stat.o enq.o deq.o
