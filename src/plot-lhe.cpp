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

void plot(const vector<string> &procdirs = { "../mc/hhmumu" });

int main(int argc, char *argv[])
{
  // Linking hack.
  ExRootTreeReader();

  // Get running directory from cmdline.
  if(argc < 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir> [ <proc-dir> ... ]" << endl;
    return 1;
  }
  plot({ &argv[1], &argv[argc] });
  return 0;
}

// Set TH1F style before drawing the histogram.
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

// Create histograms with the same nbin, xmin, and xmax for one figure.
static vector<shared_ptr<TH1F>> create_hists(const vector<string> &labels,
    Int_t nbin, Float_t xmin, Float_t xmax)
{
  vector<shared_ptr<TH1F>> hists;
  hists.reserve(labels.size());
  for(const string &label : labels) {
    hists.emplace_back(make_shared<TH1F>("", label.c_str(), nbin, xmin, xmax));
  }
  return hists;
}

// Create canvas to draw histograms.
static shared_ptr<TCanvas> create_canvas()
{
  auto canvas = make_shared<TCanvas>();
  canvas->SetTopMargin(0.02);
  canvas->SetBottomMargin(0.10);
  canvas->SetLeftMargin(0.10);
  canvas->SetRightMargin(0.02);
  return canvas;
}

// Draw and save histograms to a file.
static void draw_and_save(vector<shared_ptr<TH1F>> &hists,
    const char *path, const char *xtitle, const char *ytitle)
{
  auto canvas = create_canvas();
  for(size_t i = 0; i < hists.size(); ++i) {
    TH1F *hist = hists[i].get();
    if(hist->GetEntries() == 0) continue;
    hist->SetXTitle(xtitle);
    hist->SetYTitle(ytitle);
    hist->SetLineColor(i + 2);
    format(hist)->DrawNormalized("SAME");
  }
  TLegend *legend = canvas->BuildLegend(0.6, 0.92, 0.95, 0.8);
  legend->SetTextSize(0.03);
  canvas->SaveAs(path);
}

void plot(const vector<string> &procdirs)
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
      string lhepath = procdir + mg5run.path + "/Events/run_01/unweighted_events.root";
      clog << "INFO: processing file: \"" << lhepath << "\"" << endl;
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

      //// Print the LHEF tree.
      //if(first_mg5run) LHEF->Print();

      // Associate with branches.
      TClonesArray *events = NULL, *particles = NULL;
      //get_branch(events, LHEF, "Event", "TRootLHEFEvent") || ({ goto cleanup; false; });
      get_branch(particles, LHEF, "Particle", "TRootLHEFParticle") || ({ goto cleanup; false; });

      // Traverse tree entries.
      for(Long64_t i = 0;; ++i) {
        if(LHEF->GetEntry(i) == 0) break;

        // Traverse particles.
        Int_t npar = particles->GetEntries();
        Int_t nh = 0, nmu = 0;
        TLorentzVector ph[2], ph_sum, pmu[2], pmu_sum;
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
        if(nh > 2 || nmu != 2) {
          cerr << "ERROR: unexpected nh=" << nh << " and nmu=" << nmu << endl;
          continue;
        }
        pmu_sum = pmu[0] + pmu[1];
        pt_mu[nh]->Fill(pmu_sum.Pt());
        eta_mu[nh]->Fill(pmu_sum.Eta());
        m_mu[nh]->Fill(pmu_sum.M());
        if(nh) {
          ph_sum = ph[0] + ph[1];
          pt_h[nh]->Fill(ph_sum.Pt());
          eta_h[nh]->Fill(ph_sum.Eta());
          m_h[nh]->Fill(ph_sum.M());
        }

        //if((i + 1) % 1000 == 0) {
        //  clog << "INFO: " << setw(6) << (i + 1) << " events processed" << endl;
        //}
      }

    cleanup:
      delete events;
      delete particles;
      //first_mg5run = false;
    }
  }

  // Export histograms.
  draw_and_save(pt_mu, "pt_mu.pdf", "p_{T}^{#mu}", "density");
  draw_and_save(eta_mu, "eta_mu.pdf", "#eta^{#mu}", "density");
  draw_and_save(m_mu, "m_mu.pdf", "m_{inv}^{#mu}", "density");
  draw_and_save(pt_h, "pt_h.pdf", "p_{T}^{H}", "density");
  draw_and_save(eta_h, "eta_h.pdf", "#eta^{H}", "density");
  draw_and_save(m_h, "m_h.pdf", "m_{inv}^{H}", "density");
}
