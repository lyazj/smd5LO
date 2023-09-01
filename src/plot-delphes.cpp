#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../include/smd5/particle.h"
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

static Double_t higgs_reco1(const vector<TLorentzVector> &pb, const vector<int> &qb, vector<TLorentzVector> &ph)
{
  int nb = pb.size();
  ph = { };
  Double_t drh = INFINITY;

  // Higgs reconstruction 1/1: Which 2 b-jets to use.
  for(int b0 =  0; b0 <= nb - 2; ++b0) {
  for(int b1 = b0; b1 <= nb - 1; ++b1) {

    vector<TLorentzVector> ph_g;
    ph_g.reserve(2);
    Double_t drh_g = 0.0;
    if(qb[0] == qb[1]) continue;
    ph_g[0] = pb[0] + pb[1];
    drh_g += abs(pb[0].DeltaR(pb[1]));
    if(drh_g < drh) {
      ph = move(ph_g);
      drh = drh_g;
    }

  } }
  return drh;
}

static Double_t higgs_reco2(const vector<TLorentzVector> &pb, const vector<int> &qb, vector<TLorentzVector> &ph)
{
  int nb = pb.size();
  ph = { };
  Double_t drh = INFINITY;

  // Higgs reconstruction 1/2: Which 4 b-jets to use.
  for(int b0 =  0; b0 <= nb - 4; ++b0) {
  for(int b1 = b0; b1 <= nb - 3; ++b1) {
  for(int b2 = b1; b2 <= nb - 2; ++b2) {
  for(int b3 = b2; b3 <= nb - 1; ++b3) {
  // Higgs reconstruction 2/2: Which 2 b-jets are produced together.
  for(const auto &g : vector<vector<pair<int, int>>> { {{0,1},{2,3}}, {{0,2},{1,3}}, {{0,3},{1,2}} }) {

    vector<TLorentzVector> ph_g;
    ph_g.reserve(2);
    Double_t drh_g = 0.0;
    int ih = 0;
    for(auto [i0, i1] : g) {
      if(qb[i0] == qb[i1]) { ih = -1; break; }
      ph_g[ih++] = pb[i0] + pb[i1];
      drh_g += abs(pb[i0].DeltaR(pb[i1]));
    }
    if(ih < 0) continue;
    if(drh_g < drh) {
      ph = move(ph_g);
      drh = drh_g;
    }

  } } } } }
  return drh;
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
  vector<shared_ptr<TH1F>> pt_h = create_hists(label, nbin, pt_min, pt_max);
  vector<shared_ptr<TH1F>> eta_h = create_hists(label, nbin, eta_min, eta_max);
  vector<shared_ptr<TH1F>> m_h = create_hists(label, nbin, m_min, m_max);

  auto dumpfile = make_shared<TFile>("selected_events.root", "RECREATE");
  if(!dumpfile->IsOpen()) {
    cerr << "ERROR: error opening file to write: " << "selected_events.root" << endl;
    return;
  }
  TTree *dumptree = new TTree("Selected", "selected Delphes events");
  auto dumptree_guard = shared_ptr<TTree>(dumptree);

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

      // Additional branches.
      Int_t numHiggs; dumptree->Branch("NumHiggs", &numHiggs);
      Int_t numJet; dumptree->Branch("NumJet", &numJet);
      Int_t numBottom; dumptree->Branch("NumBottom", &numBottom);
      auto HiggsMomenta = make_shared<TClonesArray>("TLorentzVector");
      dumptree->Branch("HiggsMomenta", &HiggsMomenta);
      Double_t HiggsDeltaR; dumptree->Branch("HiggsDeltaR", &HiggsDeltaR);

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
          }
          ht += jet->PT;  // [TODO] which ones add up to Ht?
        }
        if(has_tau) continue;
        int nj = pj.size(), nb = pb.size();
        if(nj < 2) continue;
        if(nb < 2 * nh) continue;
        sort(pj.begin(), pj.end(), [](const TLorentzVector &p1, const TLorentzVector &p2) {
          return p1.Pt() > p2.Pt();
        });
        if((pj[0] + pj[1]).M() <= 700) continue;
        if(ht / pmu[0].Pt() >= 1.6) continue;  // [TODO] Change this value?
        numJet = nj, numBottom = nb;

        // Reconstruct Higgs bosons from final b-jets.
        vector<TLorentzVector> ph;
        Double_t drh = 0.0;
        if(nh == 1) {
          drh = higgs_reco1(pb, qb, ph);
        } else {
          drh = higgs_reco2(pb, qb, ph);
        }
        if((int)ph.size() != nh) {
          cerr << "ERROR: " << ph.size() << " Higgs bosons constructed, expect " << nh << endl;
          continue;
        }
        HiggsMomenta->Clear();
        for(int ih = 0; ih < nh; ++ih) new((void *)HiggsMomenta->At(ih)) TLorentzVector(ph[ih]);
        HiggsDeltaR = drh;

        // Fill dumptree and histograms.
        dumptree->Fill();
        TLorentzVector pmu_sum = pmu[0] + pmu[1];
        pt_mu[nh]->Fill(pmu_sum.Pt());
        eta_mu[nh]->Fill(pmu_sum.Eta());
        m_mu[nh]->Fill(pmu_sum.M());
        for(const TLorentzVector &p : ph) {
          pt_h[nh]->Fill(p.Pt());
          eta_h[nh]->Fill(p.Eta());
          m_h[nh]->Fill(p.M());
        }
      }

    cleanup:
      dumptree->ResetBranchAddresses();
      delete particles;
      delete electrons;
      delete muons;
      delete jets;
      delete mets;
      //first_mg5run = false;
    }

  }

  dumpfile->cd();
  dumptree->Write();

  // Export histograms.
  draw_and_save(pt_mu, "pt_mu.pdf", "p_{T}^{#mu}", "density");
  draw_and_save(eta_mu, "eta_mu.pdf", "#eta^{#mu}", "density");
  draw_and_save(m_mu, "m_mu.pdf", "m_{inv}^{#mu}", "density");
  draw_and_save(pt_h, "pt_h.pdf", "p_{T}^{#h}", "density");
  draw_and_save(eta_h, "eta_h.pdf", "#eta^{#h}", "density");
  draw_and_save(m_h, "m_h.pdf", "m_{inv}^{#h}", "density");
}
