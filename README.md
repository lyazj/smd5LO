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

## Draw diagrams and generate events

The eps viewer of `MG5_aMC@NLO` has been set as `resource/diagram-saver` to save all figures to `${PWD}/diagrams` when executing `display diagrams` in `MG5_aMC@NLO`.

In directory `mumuwwhhjj`, a shortcut named `draw-diagrams` is provided for your convenience.

Before generating events, run `make-proc` to generate `proc` directory as output of `MG5_aMC@NLO` to share between subsequent runs.

Finishing with `proc`, for each node in the cluster, edit `run-proc`, properly setting event number and other options, then execute it.

Note that both `draw-diagrams` and `make-proc` refer to the script `gen-proc`, which is the only thing to be changed when switching to another process.
