.PHONY: partytime columnprintf

all: partytime columnprintf
	@echo DONE

partytime:
	gcc -g -ggdb -O0 -c link.c -o link.o
	gcc -g -ggdb -O0 -c pidstatcmd.c -o pidstatcmd.o
	gcc -g -ggdb -O0 -c linklist.c -o linklist.o
	gcc -g -ggdb -O0 -c usrsprintf.c -o usrsprintf.o
	gcc -g -ggdb -O0 -c partytime.c -o partytime.o
	gcc -g -ggdb -O0 -c columnprintf.c -o columnprintf.o
	gcc -g -ggdb -O0 -c partycontrol.c -o partycontrol.o
	gcc -g -ggdb -O0 -o partytime -DUNIT_TEST=1 -DLinux columnprintf.o partytime.o tracebuffer.c \
	partycontrol.o link.o linklist.o usrsprintf.o pidstatcmd.o -lrt

columnprintf:
	gcc -g -ggdb -O0 -c tracebuffer.c -o tracebuffer.o
	gcc -g -ggdb -O0 -c usrsprintf.c -o usrsprintf.o
	gcc -g -ggdb -O0 -c partycontrol.c -o partycontrol.o
	gcc -g -ggdb -O0 -DUNIT_TEST=1 -DLinux tracebuffer.o usrsprintf.o partycontrol.o columnprintf.c -o columnprintf

clean:
	-rm -f *.o
	-rm -f *.a
	-rm -f partytime
	-rm -f columnprintf
