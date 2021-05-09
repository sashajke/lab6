all : task0a

task0a : task0a.c
	gcc -g -m32 -c -o task0a.o task0a.c
	gcc -g -m32 task0a.o -o mypipe
	rm task0a.o
	
.PHONY : clean
clean :
	rm mypipe
