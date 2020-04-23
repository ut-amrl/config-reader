all: example.cc
	$(CXX) --std=c++11 -Wextra -Wall -Werror -g -I ../include/ -o example example.cc -llua5.1 -lpthread
	$(CXX) --std=c++11 -Wextra -Wall -Werror -g -I ../include/ -o multi_file_example multi_file_example.cc -llua5.1 -lpthread

valgrind_run: all
	valgrind --leak-check=full ./example

clean: example
	rm example
