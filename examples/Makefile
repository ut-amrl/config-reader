all: example.cc
	clang++ --std=c++11 -Wextra -Wall -Werror -g -I ../include/ -o example example.cc -llua5.1 -lpthread

valgrind_run: all
	valgrind --leak-check=full ./example

clean: example
	rm example
