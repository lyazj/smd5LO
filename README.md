# smd5LO

A BSM model and MC analysis based on it.

## Build

Tow points of interposition are forced to proceed in building:

* `make` -> `make -j${NCPU} CXX=g++` (for all libraries)
* `g++` -> `g++ -std=c++11 -fPIC` (for `lhapdf6` only)

Before building the project, please `source thisroot.sh`.

Then use `init_*` scripts to build corresponding versions of `MG5_aMC@NLO`.

Test scripts are produced as `test-*` to validate the built.
