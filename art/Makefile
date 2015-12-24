
CXX=g++

LD=$(CXX)
AR=ar

READLINE=/opt/local
INCPATHS=-I$(READLINE)/include
LIBPATHS=-L$(READLINE)/lib

CXXFLAGS=-O2 $(INCPATHS) -Wall -std=c++11
LDFLAGS=-O2 $(LIBPATHS) -L. -lreadline -lhistory

LIBSOURCES=ast.cpp reader.cpp
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
