all : filesys clear
filesys : filesys.o
	cc -o filesys filesys.o
filesys.o : filesys.c filesys.h
	cc -c filesys.c -g filesys.c
clear:
	rm filesys.o
