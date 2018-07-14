# What is this?

This repository is a fork of the rr project originally from rr.  This fork
contains a branch (spin-off) tracks modifications to rr that allow processes to
be "spun off" of a rr replay session so that they can be attached to with and
manipulated by another tool (typically an injector provided by
[rrapper](https://github.com/pkmoore/rrapper).)  The master branch of this fork
is up to date with the latest version of rr with which these modifications can
be cleanly merged.

# Installation

rr itself should be compiled and installed according to the official
documentation except for the following:

The Python 2 extension in the rrdump directory needs to be installed somewhere
in python's module path.  This module provides functions for dumping state that
the rrapper injector needs to operate on a spun off process.

# Overview From The rr Project

rr is a lightweight tool for recording and replaying execution of applications (trees of processes and threads).  More information about the project, including instructions on how to install, run, and build rr, is at [http://rr-project.org](http://rr-project.org). The best technical overview is currently the paper [Engineering Record And Replay For Deployability: Extended Technical Report](https://arxiv.org/pdf/1705.05937.pdf).

Or go directly to the [installation and building instructions](https://github.com/mozilla/rr/wiki/Building-And-Installing).

Please contribute!  Make sure to review the [pull request checklist](/CONTRIBUTING.md) before submitting a pull request.

If you find rr useful, please [add a testimonial](https://github.com/mozilla/rr/wiki/Testimonials).

# System requirements

* Linux kernel ≥ 3.5 is required (for [seccomp-bpf](https://en.wikipedia.org/wiki/Seccomp)).
* rr currently requires an Intel CPU with [Nehalem](https://en.wikipedia.org/wiki/Nehalem_%28microarchitecture%29) (2010) or later microarchitecture.
* Running in a VM guest is supported, as long as the VM supports virtualization of hardware performance counters. (VMware and KVM are known to work; Xen works in theory but may be broken.)

