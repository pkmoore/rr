#!/bin/sh

unset CFLAGS CPATH CPPFLAGS CXXFLAGS LDFLAGS
./bootstrap
./configure --with-libunwind --with-libiberty
make clean
make -f Makefile.libstrace clean
make
make -f Makefile.libstrace
