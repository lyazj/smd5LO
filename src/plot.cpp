#include "smd5/utils.h"
#include "smd5/branch.h"
#include "ExRootClasses.h"
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TROOT.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <memory>
#include <math.h>

using namespace std;

int main(int argc, char *argv[])
{
  // Get running directory from cmdline.
  if(argc != 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir>" << endl;
    return 1;
  }
  string basedir = argv[1];
  if(basedir.empty()) basedir = ".";
  if(basedir.back() != '/') basedir.push_back('/');

  auto canvas = make_shared<TCanvas>();
  Int_t nbin = 30;
  Float_t pt_min = 0.0, pt_max = 1500.0;
  Float_t m_min = 0.0, m_max = 1500.0;
  shared_ptr<TH1F> pt_max_mu[2] = {
    make_shared<TH1F>("", "larger muon traverse momentum", nbin, pt_min, pt_max),
    make_shared<TH1F>("", "larger muon traverse momentum", nbin, pt_min, pt_max),
  };
  shared_ptr<TH1F> m_inv_mu[2] = {
    make_shared<TH1F>("", "invariant mass of muon pair", nbin, m_min, m_max),
    make_shared<TH1F>("", "invariant mass of muon pair", nbin, m_min, m_max),
  };

  // Get running info and traverse over the runs.
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

    // Traverse over tree entries.
    for(Long64_t i = 0;; ++i) {
      if(LHEF->GetEntry(i) == 0) break;

      // Traverse over particles.
      Int_t npar = particles->GetEntries();
      Int_t nh = 0, nmu = 0;
      TLorentzVector ph[2], pmu[2];
      for(Int_t j = 0; j < npar; ++j) {

        auto particle = (TRootLHEFParticle *)particles->At(j);
        if(particle->Status != 1) continue;
        if(particle->PID == 25) {
          ph[nh++].SetPtEtaPhiE(particle->PT, particle->Eta, particle->Phi, particle->E);
        }
        if(abs(particle->PID) == 13) {
          pmu[nmu++].SetPtEtaPhiE(particle->PT, particle->Eta, particle->Phi, particle->E);
        }
      }

      // Fill histograms.
      if(nh < 1 || nh > 2 || nmu != 2) {
        cerr << "ERROR: unexpected nh=" << nh << " and nmu=" << nmu << endl;
        continue;
      }
      pt_max_mu[nh - 1]->Fill(max(pmu[0].Pt(), pmu[1].Pt()));
      m_inv_mu[nh - 1]->Fill((pmu[0] + pmu[1]).M());

      if((i + 1) % 1000 == 0) {
        clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
      }
    }

  cleanup:
    delete events;
    delete particles;
    first_mg5run = false;
  }

  pt_max_mu[0]->DrawNormalized();
  pt_max_mu[1]->DrawNormalized("SAME");
  canvas->SaveAs("pt_max_mu.pdf");

  m_inv_mu[0]->DrawNormalized();
  m_inv_mu[1]->DrawNormalized("SAME");
  canvas->SaveAs("m_inv_mu.pdf");

  return 0;
}
