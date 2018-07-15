#!/bin/sh

unset CFLAGS CPATH CPPFLAGS CXXFLAGS LDFLAGS
./bootstrap
./configure --without-libunwind
make clean
make -f Makefile.libstrace clean
make
make -f Makefile.libstrace
