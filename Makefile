all:
	gcc -g -ggdb -O0 -c link.c -o link.o
	gcc -g -ggdb -O0 -c linklist.c -o linklist.o
	gcc -g -ggdb -O0 -c usrsprintf.c -o usrsprintf.o
	gcc -g -ggdb -O0 -c partytime.c -o partytime.o
	gcc -g -ggdb -O0 -o partytime -DUNIT_TEST=1 -DLinux partytime.o tracebuffer.c link.o linklist.o usrsprintf.o -lrt

clean:
	-rm -f *.o
	-rm -f *.a
	-rm -f partytime
