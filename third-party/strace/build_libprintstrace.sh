#!/bin/sh

#./bootstrap
# This always fails at the end but we need the objects in place for the next
# make
make clean
make libprintstrace.a
