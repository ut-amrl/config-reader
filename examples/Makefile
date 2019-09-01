all: example.cc
	clang++ --std=c++11 -g -I ../include/ -o example example.cc -llua5.3 -lpthread

valgrind_run: all
	valgrind --leak-check=full ./example

clean: example
	rm example