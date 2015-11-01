all : test

test : test.c coroutine.c
	gcc -g -Wall -pthread -o $@ $^

clean :
	rm test
