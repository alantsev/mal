
CXX=g++

LD=$(CXX)
AR=ar

READLINE=/opt/local
INCPATHS=-I$(READLINE)/include
LIBPATHS=-L$(READLINE)/lib

CXXFLAGS=-O3 $(INCPATHS) -Wall -std=c++14
LDFLAGS=-O3 $(LIBPATHS) -L. -lreadline -lhistory

LIBSOURCES=ast.cpp ast_details.cpp reader.cpp environment.cpp arena.cpp core.cpp
LIBOBJS=$(LIBSOURCES:%.cpp=%.o)

MAINS=$(wildcard step*.cpp)
TARGETS=$(MAINS:%.cpp=%)

.PHONY:	all clean

.SUFFIXES: .cpp .o

all: $(TARGETS)

clean:
	rm -rf *.o $(TARGETS) libmal.a .deps

.deps: *.cpp *.h
	$(CXX) $(CXXFLAGS) -MM *.cpp > .deps


.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGETS): %: %.o libmal.a
	$(LD) $^ -o $@ $(LDFLAGS)

libmal.a: $(LIBOBJS)
	$(AR) rcs $@ $^

-include .deps
