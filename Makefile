# RANS make rules. REQUIRES: gmpxx, gflags, gtest

MODE=RELEASE #or DEBUG
MATRIX_LIB=SELF #or UBLAS
RANS_USE_GFLAGS=yes

CC=g++

ifeq ($(MODE),DEBUG)
CFLAGS= -O0 -g3 -Wall -I$(PWD) -DRANS_USE_$(MATRIX_LIB) -DRANS_DEBUG
else
CFLAGS= -O3 -Wall -I$(PWD) -DRANS_USE_$(MATRIX_LIB)
endif

ifeq ($(RANS_USE_GFLAGS),yes)
CFLAGS += -DRANS_USE_GFLAGS
endif

LFLAGS=-lgmp -lgmpxx -lgflags -lgtest -lgtest_main
prefix=/usr/local

all: test rans 
rans: bin/rans
test: bin/test

check: test
	@bin/test --gtest_color=yes

bin/rans: rans.hpp test/rans.cc Makefile
	@mkdir -p bin
	$(CC) $(CFLAGS) test/rans.cc -o $@ $(LFLAGS)

bin/test: rans.hpp test/test.cc Makefile
	@mkdir -p bin
	$(CC) $(CFLAGS) test/test.cc -o $@ $(LFLAGS)

install: rans.hpp
	cp rans.hpp $(prefix)/include

install-cui: rans
	cp bin/rans $(prefix)/bin

install-all: install install-cui

uninstall:
	rm $(prefix)/include/rans.hpp $(prefix)/bin/rans

clean:
	rm -rf bin