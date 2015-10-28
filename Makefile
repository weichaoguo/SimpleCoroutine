all : test

test : test.c coroutine.c
	gcc -g -Wall -o $@ $^

clean :
	rm test
