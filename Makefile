MODE=DEVELOP    #or DEBUG

CXX=g++
ifeq ($(MODE),DEBUG)
CXXFLAGS=-O0 -g3 -Wall -DRANS_DEBUG
else
CXXFLAGS=-O3
endif
RANS_CXXFLAGS=-I${shell pwd} -lgmp -lgmpxx
GIT_REV=${shell git log -1 --format="%h"}

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
	$(CXX) $(CXXFLAGS) $(RANS_CXXFLAGS) test/rans.cc -o $@ -lgflags -DGIT_REV=\"$(GIT_REV)\"

bin/test: rans.hpp test/test.cc Makefile
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXXFLAGS) $(RANS_CXXFLAGS) test/test.cc test/gtest/gtest-all.cc test/gtest/gtest_main.cc -Itest -o $@

install: rans.hpp rans
	mkdir -p $(DESTDIR)$(includedir) $(DESTDIR)$(bindir)
	$(INSTALL_DATA) rans.hpp $(DESTDIR)$(includedir)/rans.hpp
	$(INSTALL_PROGRAM) bin/rans $(DESTDIR)$(bindir)/rans

uninstall:
	rm $(DESTDIR)$(includedir)/rans.hpp $(DESTDIR)$(bindir)/rans

clean:
	rm -rf bin
