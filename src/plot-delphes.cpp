#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../include/smd5/figure.h"
#include "../resource/MG5_aMC/Delphes/classes/DelphesClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/Delphes/libDelphes.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include "smd5/branch.h"
#include "smd5/particle.h"
#include "smd5/figure.h"
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

void plot_delphes(const vector<string> &procdirs = { "../mc/hhmumu" });

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc < 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir> [ <proc-dir> ... ]" << endl;
    return 1;
  }
  plot_delphes({ &argv[1], &argv[argc] });
  return 0;
}

void plot_delphes(const vector<string> &procdirs)
{
  // Create histograms.
  Int_t nbin = 30;
  Float_t pt_min = 0.0, pt_max = 1500.0;
  Float_t eta_min = -3.0, eta_max = 3.0;
  Float_t m_min = 0.0, m_max = 1500.0;
  vector<string> label = {
    "#font[132]{pp #rightarrow #mu^{#pm}#mu^{#pm}jj}",
    "#font[132]{pp #rightarrow H#mu^{#pm}#mu^{#pm}jj}",
    "#font[132]{pp #rightarrow HH#mu^{#pm}#mu^{#pm}jj}",
  };
  vector<shared_ptr<TH1F>> pt_mu = create_hists(label, nbin, pt_min, pt_max);
  vector<shared_ptr<TH1F>> eta_mu = create_hists(label, nbin, eta_min, eta_max);
  vector<shared_ptr<TH1F>> m_mu = create_hists(label, nbin, m_min, m_max);

  // Traverse process directories.
  for(string procdir : procdirs) {
    if(procdir.empty()) procdir = ".";
    if(procdir.back() != '/') procdir.push_back('/');
    clog << "INFO: working directory: \"" << procdir << "\"" << endl;

    // Get running info and traverse the runs.
    //bool first_mg5run = true;
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

      //// Print the Delphes tree.
      //if(first_mg5run) Delphes->Print();

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
      for(Long64_t i = 0;; ++i) {
        if(Delphes->GetEntry(i) == 0) break;

        if((i + 1) % 1000 == 0) {
          clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
        }

        // See arxiv:2012.09882, "Signal Region Cuts" on Page 4.

        if(muons->GetEntries() != 2) continue;
        if(jets->GetEntries() < 2) continue;
        if(electrons->GetEntries()) continue;
        if(((MissingET *)mets->At(0))->MET >= 30) continue;

        TLorentzVector pmu[2];
        Int_t qmu[2];
        for(Int_t j = 0; j < 2; ++j) {
          Muon *muon = (Muon *)muons->At(j);
          pmu[j] = muon->P4();
          qmu[j] = muon->Charge;
        }
        if(qmu[0] != qmu[1]) continue;
        if(pmu[0].Pt() < pmu[1].Pt()) swap(pmu[0], pmu[1]);
        if(pmu[0].Pt() <= 27) continue;
        if(pmu[1].Pt() <= 10) continue;

        Double_t ht = 0.0;
        Int_t njet = jets->GetEntries();
        vector<TLorentzVector> ptjets;
        ptjets.reserve(njet);
        bool has_tau = false;
        for(Int_t j = 0; j < njet; ++j) {
          Jet *jet = (Jet *)jets->At(j);
          if(jet->TauTag & 0b010) {  // medium WP
            has_tau = true; break;
          }
          ptjets.emplace_back(jet->P4());
          ht += jet->PT;
        }
        if(has_tau) continue;
        sort(ptjets.begin(), ptjets.end(), [](const TLorentzVector &p1, const TLorentzVector &p2) {
          return p1.Pt() > p2.Pt();
        });
        if((ptjets[0] + ptjets[1]).M() <= 700) continue;
        if(ht / pmu[0].Pt() >= 1.6) continue;

        // Get nHiggs.
        set<Int_t> sh;
        Int_t npar = particles->GetEntries();
        for(Int_t j = 0; j < npar; ++j) {
          GenParticle *particle = (GenParticle *)particles->At(j);
          int status = abs(particle->Status);
          if(status % 10 != 2) continue;  // Consider only the hard process.
          if(particle->PID != 25) continue;
          sh.insert(find_last_child(particles, j));  // Duplicate access not banned yet.
        }
        Int_t nh = sh.size();

        // Fill histograms.
        if(nh > 2) {
          cerr << "ERROR: unexpected nh=" << nh << endl;
          continue;
        }
        TLorentzVector pmu_sum = pmu[0] + pmu[1];
        pt_mu[nh]->Fill(pmu_sum.Pt());
        eta_mu[nh]->Fill(pmu_sum.Eta());
        m_mu[nh]->Fill(pmu_sum.M());
      }

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
  draw_and_save(pt_mu, "pt_mu.pdf", "p_{T}^{#mu}", "density");
  draw_and_save(eta_mu, "eta_mu.pdf", "#eta^{#mu}", "density");
  draw_and_save(m_mu, "m_mu.pdf", "m_{inv}^{#mu}", "density");
}
