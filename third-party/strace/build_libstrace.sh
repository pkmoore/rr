#!/bin/sh

unset CFLAGS CPATH CPPFLAGS CXXFLAGS LDFLAGS
./bootstrap
./configure --with-libunwind --with-libiberty
make -f Makefile.libstrace clean
make -f Makefile.libstrace all
