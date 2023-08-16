#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../resource/MG5_aMC/Delphes/classes/DelphesClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/Delphes/libDelphes.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include "smd5/branch.h"
#include <classes/DelphesClasses.h>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <TH1F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <memory>
#include <math.h>
#include <utility>
#endif  /* __CLING__ */

using namespace std;

void plot(string basedir = "../mc/hhmumu");

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc != 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir>" << endl;
    return 1;
  }
  plot(argv[1]);
  return 0;
}

static TH1F *format(TH1F *th1f)
{
  th1f->SetBit(th1f->kNoTitle | th1f->kNoStats);
  th1f->GetXaxis()->SetTitleOffset(1.2);
  th1f->GetYaxis()->SetTitleOffset(1.3);
  string title = th1f->GetTitle();
  title += " (nevt=" + to_string((Long64_t)th1f->GetEntries()) + ")";
  th1f->SetTitle(title.c_str());
  return th1f;
}

void plot(string basedir)
{
  if(basedir.empty()) basedir = ".";
  if(basedir.back() != '/') basedir.push_back('/');

  auto canvas = make_shared<TCanvas>();
  canvas->SetTopMargin(0.02);
  canvas->SetBottomMargin(0.10);
  canvas->SetLeftMargin(0.10);
  canvas->SetRightMargin(0.02);

  // Get running info and traverse the runs.
  bool first_mg5run = true;
  Long64_t cnt = 0;
  vector<Mg5Run> mg5runs = list_run(basedir);
  if(mg5runs.empty()) {
    clog << "WARNING: directory without any result: \"" << basedir << "\"" << endl;
    return;
  }
  for(const Mg5Run &mg5run : mg5runs) {
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
    TClonesArray *electrons = NULL, *muons = NULL, *jets = NULL, *mets = NULL;
    bool branches_found = false;
    do {
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

      Double_t ptmu1, ptmu2;
      if(((Muon *)muons->At(0))->Charge * ((Muon *)muons->At(1))->Charge < 0) continue;
      ptmu1 = ((Muon *)muons->At(0))->PT;
      ptmu2 = ((Muon *)muons->At(1))->PT;
      if(ptmu1 < ptmu2) swap(ptmu1, ptmu2);
      if(ptmu1 <= 27) continue;
      if(ptmu2 <= 10) continue;

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
      if(ht / ptmu1 >= 1.6) continue;

      ++cnt;
    }

  cleanup:
    delete electrons;
    delete muons;
    delete jets;
    delete mets;
    first_mg5run = false;
  }

  cout << "RESULT: " << cnt << " events remained after cutting" << endl;
}
