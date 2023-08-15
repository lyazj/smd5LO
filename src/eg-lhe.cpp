#include "smd5/utils.h"
#include "smd5/branch.h"
#include <ExRootClasses.h>
#include <ExRootTreeReader.h>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <memory>

using namespace std;

int main(int argc, char *argv[])
{
  // Linking hack.
  ExRootTreeReader();

  // Get running directory from cmdline.
  if(argc != 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir>" << endl;
    return 1;
  }
  string basedir = argv[1];
  if(basedir.empty()) basedir = ".";
  if(basedir.back() != '/') basedir.push_back('/');

  // Get running info and traverse the runs.
  bool first_mg5run = true;
  for(const Mg5Run &mg5run : list_run(basedir)) {
    string lhepath = basedir + mg5run.path + "/Events/run_01/unweighted_events.root";
    auto file = make_shared<TFile>(lhepath.c_str());
    if(!file->IsOpen()) {
      cerr << "ERROR: error opening file to read: " << lhepath << endl;
      continue;
    }

    // Get the LHEF tree.
    auto LHEF = (TTree *)file->Get("LHEF");
    if(LHEF == NULL) {
      cerr << "ERROR: error getting LHEF tree: " << lhepath << endl;
      continue;
    }

    // Print the LHEF tree.
    if(first_mg5run) LHEF->Print();

    // Associate with branches.
    TClonesArray *events = NULL, *particles = NULL;
    get_branch(events, LHEF, "Event", "TRootLHEFEvent") || ({ goto cleanup; false; });
    get_branch(particles, LHEF, "Particle", "TRootLHEFParticle") || ({ goto cleanup; false; });

    // Traverse tree entries.
    if(first_mg5run) for(Long64_t i = 0; i < 100; ++i) {
      if(LHEF->GetEntry(i) == 0) break;
      cout << setw(8) << left << i << right;

      // Traverse particles.
      Int_t npar = particles->GetEntries();
      for(Int_t j = 0; j < npar; ++j) {
        auto particle = (TRootLHEFParticle *)particles->At(j);
        if(particle->Status != 1) continue;
        cout << setw(8) << particle->PID;
      }
      cout << endl;
    }

  cleanup:
    delete events;
    delete particles;
    first_mg5run = false;
  }
  return 0;
}
