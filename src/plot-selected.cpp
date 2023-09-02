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
#endif  /* __CLING__ */

using namespace std;

void plot_selected(const vector<string> &procdirs = { "../mc/hhmumu" });

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc < 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir> [ <proc-dir> ... ]" << endl;
    return 1;
  }
  plot_selected({ &argv[1], &argv[argc] });
  return 0;
}

void plot_selected(const vector<string> &procdirs)
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
  vector<shared_ptr<TH1F>> pt_h = create_hists(label, nbin, pt_min, pt_max);
  vector<shared_ptr<TH1F>> eta_h = create_hists(label, nbin, eta_min, eta_max);
  vector<shared_ptr<TH1F>> m_h = create_hists(label, nbin, m_min, m_max);

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
    for(const string &rootpath : mg5run.selected()) {
      string lhepath = procdir + rootpath;
      clog << "INFO: processing file: \"" << lhepath << "\"" << endl;
      auto file = make_shared<TFile>(lhepath.c_str());
      if(!file->IsOpen()) {
        cerr << "ERROR: error opening file to read: " << lhepath << endl;
        continue;
      }

      // Get the Selected tree.
      auto Selected = (TTree *)file->Get("Selected");
      if(Selected == NULL) {
        cerr << "ERROR: error getting Selected tree: " << lhepath << endl;
        continue;
      }

      //// Print the Selected tree.
      //if(first_mg5run) Selected->Print();

      // Associate with branches.
      TClonesArray *muons, *HiggsMomenta = NULL;
      Int_t numHiggs;
      bool branches_found = false;
      do {
        get_branch(muons, Selected, "Muon", "Muon") || ({ break; false; });
        !Selected->SetBranchAddress("NumHiggs", &numHiggs) || ({ break; false; });
        get_branch(HiggsMomenta, Selected, "HiggsMomenta", "TLorentzVector") || ({ break; false; });
        branches_found = true;
      } while(false);
      if(!branches_found) {
        cerr << "ERROR: error opening file to read: " << lhepath << endl;
        goto cleanup;
      }

      // Traverse tree entries.
      for(Long64_t i = 0;; ++i) {
        if(Selected->GetEntry(i) == 0) break;

        if((i + 1) % 1000 == 0) {
          clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
        }

        TLorentzVector pmu[2];
        for(Int_t j = 0; j < 2; ++j) {
          Muon *muon = (Muon *)muons->At(j);
          pmu[j] = muon->P4();
        }
        int nh = numHiggs;

        // Fill histograms.
        TLorentzVector pmu_sum = pmu[0] + pmu[1];
        pt_mu[nh]->Fill(pmu_sum.Pt());
        eta_mu[nh]->Fill(pmu_sum.Eta());
        m_mu[nh]->Fill(pmu_sum.M());
        for(int ih = 0; ih < nh; ++ih) {
          TLorentzVector *p = (TLorentzVector *)HiggsMomenta->At(ih);
          pt_h[nh]->Fill(p->Pt());
          eta_h[nh]->Fill(p->Eta());
          m_h[nh]->Fill(p->M());
        }
      }

    cleanup:
      delete muons;
      delete HiggsMomenta;
      //first_mg5run = false;
    } }

  }

  // Export histograms.
  draw_and_save(pt_mu, "pt_mu.pdf", "p_{T}^{#mu}", "density");
  draw_and_save(eta_mu, "eta_mu.pdf", "#eta^{#mu}", "density");
  draw_and_save(m_mu, "m_mu.pdf", "m_{inv}^{#mu}", "density");
  draw_and_save(pt_h, "pt_h.pdf", "p_{T}^{h}", "density");
  draw_and_save(eta_h, "eta_h.pdf", "#eta^{h}", "density");
  draw_and_save(m_h, "m_h.pdf", "m_{inv}^{h}", "density");
}
