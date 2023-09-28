#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../include/smd5/particle.h"
#include "../include/smd5/reco.h"
#include "../resource/MG5_aMC/Delphes/classes/DelphesClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/Delphes/libDelphes.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include "smd5/branch.h"
#include "smd5/particle.h"
#include "smd5/reco.h"
#include <classes/DelphesClasses.h>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <TLorentzVector.h>
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <memory>
#include <math.h>
#include <set>
#endif  /* __CLING__ */

using namespace std;

void delphes_select(const char *rootpath, const char *dumppath);

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc != 3) {
    cerr << "usage: " << program_invocation_short_name << " <rootpath> <dumppath>" << endl;
    return 1;
  }
  delphes_select(argv[1], argv[2]);
  return 0;
}

void delphes_select(const char *rootpath, const char *dumppath)
{
  auto dumpfile = make_shared<TFile>(dumppath, "RECREATE");
  if(!dumpfile->IsOpen()) {
    cerr << "ERROR: error opening file to write: " << dumppath << endl;
    return;
  }
  TTree *dumptree = new TTree("Selected", "selected Delphes events");
  auto dumptree_guard = shared_ptr<TTree>(dumptree);

  // Additional branches.
  Int_t numHiggs; dumptree->Branch("NumHiggs", &numHiggs);
  Int_t numJet; dumptree->Branch("NumJet", &numJet);
  Int_t numBottom; dumptree->Branch("NumBottom", &numBottom);
  auto HiggsMomenta = new TClonesArray("TLorentzVector");
  dumptree->Branch("HiggsMomenta", &HiggsMomenta);
  Double_t HiggsDeltaR; dumptree->Branch("HiggsDeltaR", &HiggsDeltaR);

  clog << "INFO: processing file: \"" << rootpath << "\"" << endl;
  auto file = make_shared<TFile>(rootpath);
  if(!file->IsOpen()) {
    cerr << "ERROR: error opening file to read: " << rootpath << endl;
    return;
  }

  // Get the Delphes tree.
  auto Delphes = (TTree *)file->Get("Delphes");
  if(Delphes == NULL) {
    cerr << "ERROR: error getting Delphes tree: " << rootpath << endl;
    return;
  }

  // Associate with branches.
  TClonesArray *particles, *electrons = NULL, *muons = NULL, *jets = NULL, *mets = NULL;
  bool branches_found = false;
  do {
    get_branch(particles, Delphes, "Particle", "GenParticle") || ({ break; false; });
    get_branch(electrons, Delphes, "Electron", "Electron") || ({ break; false; });
    get_branch(muons, Delphes, dumptree, "Muon", "Muon") || ({ break; false; });
    get_branch(jets, Delphes, dumptree, "Jet", "Jet") || ({ break; false; });
    get_branch(mets, Delphes, dumptree, "MissingET", "MissingET") || ({ break; false; });
    branches_found = true;
  } while(false);
  if(!branches_found) {
    cerr << "ERROR: error opening file to read: " << rootpath << endl;
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
#if 0  // Disable fake neutrino veto.
    if(((MissingET *)mets->At(0))->MET >= 30) continue;
#endif

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
    if(nh > 2) {
      cerr << "ERROR: unexpected nh=" << nh << endl;
      continue;
    }
    numHiggs = nh;

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
        ht += jet->PT;  // b-jets excluded
      }
    }
    if(has_tau) continue;
    int nj = pj.size(), nb = pb.size();
    if(nj < 2) continue;
    if(nb < 2 * nh) continue;
    sort(pj.begin(), pj.end(), [](const TLorentzVector &p1, const TLorentzVector &p2) {
      return p1.Pt() > p2.Pt();
    });
    if((pj[0] + pj[1]).M() <= 700) continue;
    //if(ht / pmu[0].Pt() >= 1.6) continue;
    numJet = nj, numBottom = nb;

    // Reconstruct Higgs bosons from final b-jets.
    vector<TLorentzVector> ph;
    Double_t drh = 0.0;
    if(nh == 1) {
      drh = higgs_reco1(pb, qb, ph);
    } else {
      drh = higgs_reco2(pb, qb, ph);
    }
    if((int)ph.size() < nh) {  // [NOTE] reconstruction MAY fail
      //cerr << "WARNING: " << ph.size() << " Higgs bosons constructed, expect " << nh << endl;
      continue;
    }
    HiggsMomenta->Clear();
    for(int ih = 0; ih < nh; ++ih) new((void *)HiggsMomenta->operator[](ih)) TLorentzVector(ph[ih]);
    HiggsDeltaR = drh;

    dumptree->Fill();
  }

cleanup:
  delete particles;
  delete electrons;
  delete muons;
  delete jets;
  delete mets;

  dumpfile->cd();
  dumptree->Write();
  delete HiggsMomenta;
}
