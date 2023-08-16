#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../include/smd5/branch.h"
#include "../resource/MG5_aMC/ExRootAnalysis/ExRootAnalysis/ExRootClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/ExRootAnalysis/libExRootAnalysis.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include "smd5/branch.h"
#include <ExRootClasses.h>
#include <ExRootTreeReader.h>
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
#endif  /* __CLING__ */

using namespace std;

void plot(string basedir = "../mc/hhmumu");

int main(int argc, char *argv[])
{
  // Linking hack.
  ExRootTreeReader();

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

  Int_t nbin = 30;
  Float_t pt_min = 0.0, pt_max = 1500.0;
  Float_t eta_min = -3.0, eta_max = 3.0;
  Float_t m_min = 0.0, m_max = 1500.0;
  const char *label[2] = {
    "\\font[132]{pp \\rightarrow H\\mu^{\\pm}\\mu^{\\pm}jj}",
    "\\font[132]{pp \\rightarrow HH\\mu^{\\pm}\\mu^{\\pm}jj}",
  };
  shared_ptr<TH1F> pt_mu[2] = {
    make_shared<TH1F>("", label[0], nbin, pt_min, pt_max),
    make_shared<TH1F>("", label[1], nbin, pt_min, pt_max),
  };
  shared_ptr<TH1F> eta_mu[2] = {
    make_shared<TH1F>("", label[0], nbin, eta_min, eta_max),
    make_shared<TH1F>("", label[1], nbin, eta_min, eta_max),
  };
  shared_ptr<TH1F> m_mu[2] = {
    make_shared<TH1F>("", label[0], nbin, m_min, m_max),
    make_shared<TH1F>("", label[1], nbin, m_min, m_max),
  };
  shared_ptr<TH1F> pt_h[2] = {
    make_shared<TH1F>("", label[0], nbin, pt_min, pt_max),
    make_shared<TH1F>("", label[1], nbin, pt_min, pt_max),
  };
  shared_ptr<TH1F> eta_h[2] = {
    make_shared<TH1F>("", label[0], nbin, eta_min, eta_max),
    make_shared<TH1F>("", label[1], nbin, eta_min, eta_max),
  };
  shared_ptr<TH1F> m_h[2] = {
    make_shared<TH1F>("", label[0], nbin, m_min, m_max),
    make_shared<TH1F>("", label[1], nbin, m_min, m_max),
  };

  // Get running info and traverse the runs.
  bool first_mg5run = true;
  vector<Mg5Run> mg5runs = list_run(basedir);
  if(mg5runs.empty()) {
    clog << "WARNING: directory without any result: \"" << basedir << "\"" << endl;
    return;
  }
  for(const Mg5Run &mg5run : mg5runs) {
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
    for(Long64_t i = 0;; ++i) {
      if(LHEF->GetEntry(i) == 0) break;

      // Traverse particles.
      Int_t npar = particles->GetEntries();
      Int_t nh = 0, nmu = 0;
      TLorentzVector ph[2], pmu[2];
      for(Int_t j = 0; j < npar; ++j) {

        auto particle = (TRootLHEFParticle *)particles->At(j);
        if(particle->Status != 1) continue;
        if(particle->PID == 25) {
          if(++nh <= 2) ph[nh - 1].SetPtEtaPhiE(particle->PT, particle->Eta, particle->Phi, particle->E);
        }
        if(abs(particle->PID) == 13) {
          if(++nmu <= 2) pmu[nmu - 1].SetPtEtaPhiE(particle->PT, particle->Eta, particle->Phi, particle->E);
        }
      }

      // Fill histograms.
      if(nh < 1 || nh > 2 || nmu != 2) {
        cerr << "ERROR: unexpected nh=" << nh << " and nmu=" << nmu << endl;
        continue;
      }
      pt_mu[nh - 1]->Fill((pmu[0] + pmu[1]).Pt());
      eta_mu[nh - 1]->Fill((pmu[0] + pmu[1]).Eta());
      m_mu[nh - 1]->Fill((pmu[0] + pmu[1]).M());
      pt_h[nh - 1]->Fill((ph[0] + ph[1]).Pt());
      eta_h[nh - 1]->Fill((ph[0] + ph[1]).Eta());
      m_h[nh - 1]->Fill((ph[0] + ph[1]).M());

      if((i + 1) % 1000 == 0) {
        clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
      }
    }

  cleanup:
    delete events;
    delete particles;
    first_mg5run = false;
  }

  TLegend *legend;

  for(int i = 0; i < 2; ++i) {
    pt_mu[i]->SetXTitle("p_{T}^{\\mu}");
    pt_mu[i]->SetYTitle("density");
    pt_mu[i]->SetLineColor(i + 2);
    format(pt_mu[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("pt_max_mu.pdf");

  for(int i = 0; i < 2; ++i) {
    eta_mu[i]->SetXTitle("\\eta^{\\mu}");
    eta_mu[i]->SetYTitle("density");
    eta_mu[i]->SetLineColor(i + 2);
    format(eta_mu[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("eta_mu.pdf");

  for(int i = 0; i < 2; ++i) {
    m_mu[i]->SetXTitle("m_{inv}^{\\mu}");
    m_mu[i]->SetYTitle("density");
    m_mu[i]->SetLineColor(i + 2);
    format(m_mu[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("m_mu.pdf");

  for(int i = 0; i < 2; ++i) {
    pt_h[i]->SetXTitle("p_{T}^{H}");
    pt_h[i]->SetYTitle("density");
    pt_h[i]->SetLineColor(i + 2);
    format(pt_h[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("pt_h.pdf");

  for(int i = 0; i < 2; ++i) {
    eta_h[i]->SetXTitle("\\eta^{H}");
    eta_h[i]->SetYTitle("density");
    eta_h[i]->SetLineColor(i + 2);
    format(eta_h[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("eta_h.pdf");

  for(int i = 0; i < 2; ++i) {
    m_h[i]->SetXTitle("m_{inv}^{H}");
    m_h[i]->SetYTitle("density");
    m_h[i]->SetLineColor(i + 2);
    format(m_h[i].get())->DrawNormalized(i == 0 ? "" : "SAME");
  }
  legend = canvas->BuildLegend(0.6, 0.9, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs("m_h.pdf");
}
