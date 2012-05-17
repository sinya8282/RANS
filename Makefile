# RANS make rules. REQUIRES: gmpxx, gflags, gtest

MODE=DEVELOP    #or DEBUG
MATRIX_LIB=SELF #or UBLAS

CXX=g++

ifeq ($(MODE),DEBUG)
CFLAGS= -O0 -g3 -Wall -I${shell pwd} -DRANS_USE_$(MATRIX_LIB) -DRANS_DEBUG
else
CFLAGS= -O3 -I${shell pwd} -DRANS_USE_$(MATRIX_LIB)
endif

LFLAGS=-lgmp -lgmpxx
prefix=/usr/local

rans: bin/rans
test: bin/test

all: rans test

check: test
	@bin/test --gtest_color=yes

bin/rans: rans.hpp test/rans.cc Makefile
	@mkdir -p bin
	$(CXX) $(CFLAGS) test/rans.cc -o $@ $(LFLAGS) -lgflags

bin/test: rans.hpp test/test.cc Makefile
	@mkdir -p bin
	$(CXX) $(CFLAGS) test/test.cc -o $@ $(LFLAGS) -lgtest -lgtest_main -lpthread

install: rans.hpp rans
	ls $(PWD)
	ls ${shell pwd}
	ls bin
	cp bin/rans $(prefix)/bin

uninstall:
	rm $(prefix)/include/rans.hpp $(prefix)/bin/rans

clean:
	rm -rf bin