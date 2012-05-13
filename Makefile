#MATRIX_LIB=UBLAS
CC=g++
CFLAGS=-O3 -g3 -Wall -I$(PWD) -I$(PWD)/test -DRANS_USE_$(MATRIX_LIB)
LFLAGS=-lgmp -lgmpxx -lgflags
prefix=/usr/local

all: test rans 
rans: bin/rans
test: bin/test

check: test
	@bin/test

bin/rans: rans.hpp test/rans.cc Makefile
	@mkdir -p bin
	$(CC) $(CFLAGS) test/rans.cc -o $@ $(LFLAGS)

bin/test: rans.hpp test/test.cc Makefile
	@mkdir -p bin
	$(CC) $(CFLAGS) test/test.cc test/gtest-all.cc test/gtest_main.cc -o $@ $(LFLAGS)

install: rans.hpp
	cp rans.hpp $(prefix)/include

install-cui: rans
	cp bin/rans $(prefix)/bin

install-all: install install-cui

uninstall:
	rm $(prefix)/include/rans.hpp $(prefix)/bin/rans

clean:
	rm -rf bin