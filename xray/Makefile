OPTS=-std=c++17 -Wall -O3

all: solver-xray

solver-xray: bridge-solver.cc
	g++ $(OPTS) -o $@ $^

test: solver-xray
	./run_solver_tests.sh

clean:
	rm -f solver-xray
