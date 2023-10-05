obj = tmp/cdcl.o tmp/cnf.o tmp/main.o
flags = -g2 -Wall -O2 -std=c++17

bin/main: $(obj)
	g++ $(obj) -o $@ $(flags)

tmp/cdcl.o: cdcl.cpp cdcl.hpp cnf.hpp
	g++ -c $< -o $@ $(flags)

tmp/cnf.o: cnf.cpp cnf.hpp 
	g++ -c $< -o $@ $(flags)

tmp/main.o: main.cpp cnf.hpp
	g++ -c $< -o $@ $(flags)

clean:
	rm -r tmp/ bin/

init:
	@ mkdir -p bin tmp

all: init bin/main
