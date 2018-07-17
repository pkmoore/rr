#!/bin/sh

make clean
make -f Makefile.libstrace clean
make
make -f Makefile.libstrace libprintstrace.a
