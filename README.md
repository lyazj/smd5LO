# smd5LO

A BSM model and MC analysis based on it.

## Build

The building scripts are designed in general consideration, but till now only tuned and tested on PKU farm.

Tow points of interposition are forced to proceed in building:

* `make` -> `make -j${NCPU}` (for all libraries)
* `g++` -> `g++ -std=c++11 -fPIC` (for `lhapdf6` only)

Before building the project, please `source thisroot.sh`.

Then use `init_*` scripts to build corresponding versions of `MG5_aMC@NLO`.

Test scripts are produced as `test-*` to validate the built.
