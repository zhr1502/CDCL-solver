obj = tmp/cdcl.o tmp/cnf.o
vobj = tmp/verify.o
flags = -g2 -Wall -O2 -std=c++17

bin/main: $(obj) tmp/main.o
	g++ $(obj) tmp/main.o -o $@ $(flags)

bin/verify: $(vobj) $(obj)
	g++ $(vobj) $(obj) -o $@ $(flags)

tmp/verify.o: src/verifier/verify.cpp src/verifier/check_utility.hpp
	g++ -c $< -o $@ $(flags)

tmp/cdcl.o: src/cdcl.cpp src/cdcl.hpp src/cnf.hpp
	g++ -c $< -o $@ $(flags)

tmp/cnf.o: src/cnf.cpp src/cnf.hpp 
	g++ -c $< -o $@ $(flags)

tmp/main.o: src/main.cpp src/cnf.hpp
	g++ -c $< -o $@ $(flags)

clean:
	rm -r tmp/ bin/

init:
	@ mkdir -p bin tmp

all: init bin/main bin/verify
