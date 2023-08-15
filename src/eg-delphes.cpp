#include "smd5/utils.h"
#include "smd5/branch.h"
#include <classes/DelphesClasses.h>
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
  GenParticle();

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
    string lhepath = basedir + mg5run.path + "/Events/run_01/tag_1_delphes_events.root";
    auto file = make_shared<TFile>(lhepath.c_str());
    if(!file->IsOpen()) {
      cerr << "ERROR: error opening file to read: " << lhepath << endl;
      continue;
    }

    // Get the Delphes tree.
    auto Delphes = (TTree *)file->Get("Delphes");
    if(Delphes == NULL) {
      cerr << "ERROR: error getting Delphes tree: " << lhepath << endl;
      continue;
    }

    // Print the Delphes tree.
    if(first_mg5run) Delphes->Print();

    // Associate with branches.
    TClonesArray *events = NULL, *particles = NULL;
    get_branch(particles, Delphes, "Particle", "GenParticle") || ({ goto cleanup; false; });

    // Traverse tree entries.
    if(first_mg5run) for(Long64_t i = 0; i < 100; ++i) {
      if(Delphes->GetEntry(i) == 0) break;
      cout << setw(8) << left << i << right;

      // Traverse particles.
      Int_t npar = particles->GetEntries();
      for(Int_t j = 0; j < npar; ++j) {
        auto particle = (GenParticle *)particles->At(j);
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
