obj = tmp/cdcl.o tmp/cnf.o
vobj = tmp/verify.o
makefile = Makefile
flags = -Wall -g2 -O2 -std=c++17

bin/main: $(obj) tmp/main.o Makefile
	g++ $(obj) tmp/main.o -o $@ $(flags)

bin/verify: $(vobj) $(obj) Makefile
	g++ $(vobj) $(obj) -o $@ $(flags)

tmp/verify.o: src/verifier/verify.cpp src/verifier/check_utility.hpp Makefile
	g++ -c $< -o $@ $(flags)

tmp/cdcl.o: src/cdcl.cpp src/cdcl.hpp src/cnf.hpp Makefile
	g++ -c $< -o $@ $(flags)

tmp/cnf.o: src/cnf.cpp src/cnf.hpp Makefile
	g++ -c $< -o $@ $(flags)

tmp/main.o: src/main.cpp src/cnf.hpp Makefile
	g++ -c $< -o $@ $(flags)

clean:
	rm -r tmp/ bin/

init:
	@ mkdir -p bin tmp

all: init bin/main bin/verify
