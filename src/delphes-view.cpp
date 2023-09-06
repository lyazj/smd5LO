#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../include/smd5/particle.h"
#include "../include/smd5/figure.h"
#include "../include/smd5/reco.h"
#include "../resource/MG5_aMC/Delphes/classes/DelphesClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/Delphes/libDelphes.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include "smd5/branch.h"
#include "smd5/particle.h"
#include "smd5/figure.h"
#include "smd5/reco.h"
#include <classes/DelphesClasses.h>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <TH1F.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <memory>
#include <math.h>
#include <set>
#endif  /* __CLING__ */

using namespace std;

void delphes_view(const vector<string> &procdirs = { "../mc/hhmumu" });

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc < 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir> [ <proc-dir> ... ]" << endl;
    return 1;
  }
  delphes_view({ &argv[1], &argv[argc] });
  return 0;
}

void delphes_view(const vector<string> &procdirs)
{
  // Create histograms.
  Int_t nbin = 30;
  Float_t pt_min = 0.0, pt_max = 1500.0;
  Float_t eta_min = -3.0, eta_max = 3.0;
  Float_t m_min = 0.0, m_max = 1500.0;
  auto hist_nmu = make_shared<TH1F>("", "", nbin, 0,  5);
  auto hist_nj  = make_shared<TH1F>("", "", nbin, 0, 15);
  auto hist_ne  = make_shared<TH1F>("", "", nbin, 0,  5);

  // Traverse process directories.
  Long64_t ievt = 0;
  for(string procdir : procdirs) {
    if(procdir.empty()) procdir = ".";
    if(procdir.back() != '/') procdir.push_back('/');
    clog << "INFO: working directory: \"" << procdir << "\"" << endl;

    vector<Mg5Run> mg5runs = list_run(procdir);
    if(mg5runs.empty()) {
      clog << "WARNING: directory without any result: \"" << procdir << "\"" << endl;
      continue;
    }
    for(const Mg5Run &mg5run : mg5runs) {
      string lhepath = procdir + mg5run.path + "/Events/run_01/tag_1_delphes_events.root";
      clog << "INFO: processing file: \"" << lhepath << "\"" << endl;
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

      // Associate with branches.
      TClonesArray *particles, *electrons = NULL, *muons = NULL, *jets = NULL, *mets = NULL;
      bool branches_found = false;
      do {
        get_branch(particles, Delphes, "Particle", "GenParticle") || ({ break; false; });
        get_branch(electrons, Delphes, "Electron", "Electron") || ({ break; false; });
        get_branch(muons, Delphes, "Muon", "Muon") || ({ break; false; });
        get_branch(jets, Delphes, "Jet", "Jet") || ({ break; false; });
        get_branch(mets, Delphes, "MissingET", "MissingET") || ({ break; false; });
        branches_found = true;
      } while(false);
      if(!branches_found) {
        cerr << "ERROR: error opening file to read: " << lhepath << endl;
        goto cleanup;
      }

      // Traverse tree entries.
      for(Long64_t i = 0; ievt < 100000; ++i, ++ievt) {
        if(Delphes->GetEntry(i) == 0) break;

        if((i + 1) % 1000 == 0) {
          clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
        }
      }
      hist_nmu->Fill(muons->GetEntries());
      hist_nj->Fill(jets->GetEntries());
      hist_ne->Fill(electrons->GetEntries());

    cleanup:
      delete particles;
      delete electrons;
      delete muons;
      delete jets;
      delete mets;
      //first_mg5run = false;
    }

  }

  // Export histograms.
  draw_and_save(hist_nmu, "nmu.pdf", "N^{#mu}", "density");
  draw_and_save(hist_nj,  "nj.pdf",  "N^{j}", "density");
  draw_and_save(hist_ne,  "ne.pdf",  "N^{e}", "density");
}
