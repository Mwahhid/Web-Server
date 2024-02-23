CC=clang
CFLAGS=-g -Wall -Werror -Wno-unused

httpserver: httpserver.o mime.o comm.o
	clang -o $@ $^

clean:
	rm -f httpserver *.o
	rm -fr *.dSYM
