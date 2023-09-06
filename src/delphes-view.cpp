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
#include <TCanvas.h>
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
  Float_t m_min = 0.0, m_max = 1500.0;
  auto hist_nmu    = make_shared<TH1F>("", "", nbin, 0, 5);
  auto hist_nj     = make_shared<TH1F>("", "", nbin, 0, 15);
  auto hist_nb     = make_shared<TH1F>("", "", nbin, 0, 8);
  auto hist_mj     = make_shared<TH1F>("", "", nbin, m_min, m_max);
  auto hist_hjptmu = make_shared<TH1F>("", "", nbin, 0, 2);
  auto hist_ne     = make_shared<TH1F>("", "", nbin, 0, 5);
  auto hist_met    = make_shared<TH1F>("", "", nbin, pt_min, pt_max);
  auto hist_ptmu1  = make_shared<TH1F>("", "", nbin, pt_min, pt_max);
  auto hist_ptmu2  = make_shared<TH1F>("", "", nbin, pt_min, pt_max);
  auto hist_hastau = make_shared<TH1F>("", "", nbin, 0, 2);

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
      for(Long64_t i = 0; ievt < 10000; ++i, ++ievt) {
        if(Delphes->GetEntry(i) == 0) break;

        if((i + 1) % 1000 == 0) {
          clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
        }

        hist_nmu->Fill(muons->GetEntries());
        hist_ne->Fill(electrons->GetEntries());
        hist_met->Fill(((MissingET *)mets->At(0))->MET);

        do {
          if(muons->GetEntries() != 2) continue;
          TLorentzVector pmu[2];
          Int_t qmu[2];
          for(Int_t j = 0; j < 2; ++j) {
            Muon *muon = (Muon *)muons->At(j);
            pmu[j] = muon->P4();
            qmu[j] = muon->Charge;
          }
          if(qmu[0] != qmu[1]) continue;
          if(pmu[0].Pt() < pmu[1].Pt()) swap(pmu[0], pmu[1]);
          hist_ptmu1->Fill(pmu[0].Pt());
          hist_ptmu2->Fill(pmu[1].Pt());

          Double_t ht = 0.0;
          Int_t njet = jets->GetEntries();
          vector<TLorentzVector> pj;
          vector<TLorentzVector> pb;
          vector<Int_t> qb;
          bool has_tau = false;
          for(Int_t j = 0; j < njet; ++j) {
            Jet *jet = (Jet *)jets->At(j);
            if(jet->TauTag) {  // only one WP
              has_tau = true; break;
            }
            if(jet->BTag) {  // only one WP
              pb.emplace_back(jet->P4());
              qb.push_back(jet->Charge);
            } else {
              pj.emplace_back(jet->P4());
            }
            ht += jet->PT;  // [TODO] which ones add up to Ht?
          }
          hist_hastau->Fill(has_tau);
          if(has_tau) continue;
          int nh = 2, nj = pj.size(), nb = pb.size();
          hist_nj->Fill(nj);
          hist_nb->Fill(nb);
          if(nj < 2) continue;
          if(nb < 2 * nh) continue;
          sort(pj.begin(), pj.end(), [](const TLorentzVector &p1, const TLorentzVector &p2) {
            return p1.Pt() > p2.Pt();
          });
          hist_mj->Fill((pj[0] + pj[1]).M());
          hist_hjptmu->Fill(ht / pmu[0].Pt());
        } while(0);
      }

    cleanup:
      delete particles;
      delete electrons;
      delete muons;
      delete jets;
      delete mets;
    }

  }

  // Export histograms.
  auto pp = [](TCanvas *c) { c->SetLogy(); };
  draw_and_save(hist_nmu,    "nmu.pdf",    "N^{#mu}",              "density", pp);
  draw_and_save(hist_nj,     "nj.pdf",     "N^{j}",                "density", pp);
  draw_and_save(hist_nb,     "nb.pdf",     "N^{b}",                "density", pp);
  draw_and_save(hist_mj,     "mj.pdf",     "m^{jets} [GeV]",       "density", pp);
  draw_and_save(hist_hjptmu, "hjptmu.pdf", "H^{jets}/p_{T}^{#mu}", "density", pp);
  draw_and_save(hist_ne,     "ne.pdf",     "N^{e}",                "density", pp);
  draw_and_save(hist_met,    "met.pdf",    "MET [GeV]",            "density", pp);
  draw_and_save(hist_ptmu1,  "ptmu1.pdf",  "P_{T}^{#mu1} [GeV]",   "density", pp);
  draw_and_save(hist_ptmu2,  "ptmu2.pdf",  "P_{T}^{#mu2} [GeV]",   "density", pp);
  draw_and_save(hist_hastau, "hastau.pdf", "has #tau",             "density", pp);
}
