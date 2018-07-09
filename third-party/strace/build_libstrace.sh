#!/bin/bash

unset CFLAGS CPATH CPPFLAGS CXXFLAGS LDFLAGS
./bootstrap
make clean
make all
