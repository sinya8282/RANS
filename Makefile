MODE=DEVELOP    #or DEBUG
MATRIX_LIB=SELF #or UBLAS

CXX=g++
ifeq ($(MODE),DEBUG)
CXXFLAGS= -O0 -g3 -Wall -I${shell pwd} -DRANS_USE_$(MATRIX_LIB) -DRANS_DEBUG
else
CXXFLAGS= -O3 -I${shell pwd} -DRANS_USE_$(MATRIX_LIB)
endif

prefix=/usr/local
exec_prefix=$(prefix)
bindir=$(exec_prefix)/bin
includedir=$(prefix)/include
libdir=$(exec_prefix)/lib
INSTALL=install
INSTALL_PROGRAM=$(INSTALL)
INSTALL_DATA=$(INSTALL) -m 644

rans: bin/rans
test: bin/test

all: rans test

check: test
	@bin/test --gtest_color=yes

bin/rans: rans.hpp test/rans.cc Makefile
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) test/rans.cc -o $@ $(LFLAGS) -lgflags -lgmp -lgmpxx

bin/test: rans.hpp test/test.cc Makefile
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) test/test.cc -o $@ $(LFLAGS) -lgtest -lgtest_main -lpthread -lgmp -lgmpxx

install: rans.hpp rans
	mkdir -p $(DESTDIR)$(includedir) $(DESTDIR)$(bindir)

# $(INSTALL_DATA) rans.hpp $(DESTDIR)$(includedir)/rans.hpp
# $(INSTALL_PROGRAM) bin/rans $(DESTDIR)$(bindir)/rans

uninstall:
	rm $(DESTDIR)$(includedir)/rans.hpp $(DESTDIR)$(bindir)/rans

clean:
	rm -rf bin