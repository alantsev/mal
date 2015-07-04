
CXX=g++

LD=$(CXX)
AR=ar

CXXFLAGS=-O2 -Wall -std=c++11
LDFLAGS=-O2  -L.

step0_repl.o: step0_repl.cpp
	$(CXX) $(CXXFLAGS) -c step0_repl.cpp

step0_repl: step0_repl.o
	$(LD) $(LDFLAGS) -o step0_repl step0_repl.o