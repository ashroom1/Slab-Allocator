all: slabbench

slabbench: libmull.so
	cc -O3 -L. -I. -o slabbench slabbench.c -lmull

libmull.so: mull.o
	cc -shared -o libmull.so mull.o

mull.o: mull.c
	cc -c -fpic mull.c
      	
clean:
	rm -f mull.o libmull.so slabbench
