#!/bin/sh

./bootstrap
make
make -f Makefile.libstrace
