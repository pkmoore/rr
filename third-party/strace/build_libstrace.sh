#!/bin/bash

unset CFLAGS CPATH CPPFLAGS CXXFLAGS LDFLAGS
./bootstrap
make -f Makefile.libstrace clean
make -f Makefile.libstrace all
