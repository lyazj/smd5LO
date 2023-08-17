#include "smd5/figure.h"
#include <TH1F.h>
#include <TCanvas.h>
#include <TLegend.h>

using namespace std;

TH1F *format(TH1F *th1f)
{
  th1f->SetBit(th1f->kNoTitle | th1f->kNoStats);
  th1f->GetXaxis()->SetTitleOffset(1.2);
  th1f->GetYaxis()->SetTitleOffset(1.3);

  // Append entry number to the title for legend use.
  string title = th1f->GetTitle();
  title += " (nevt=" + to_string((Long64_t)th1f->GetEntries()) + ")";
  th1f->SetTitle(title.c_str());

  return th1f;
}

vector<shared_ptr<TH1F>> create_hists(const vector<string> &labels,
    int nbin, float xmin, float xmax)
{
  vector<shared_ptr<TH1F>> hists;
  hists.reserve(labels.size());
  for(const string &label : labels) {
    hists.emplace_back(make_shared<TH1F>("", label.c_str(), nbin, xmin, xmax));
  }
  return hists;
}

shared_ptr<TCanvas> create_canvas()
{
  auto canvas = make_shared<TCanvas>();
  canvas->SetTopMargin(0.02);
  canvas->SetBottomMargin(0.10);
  canvas->SetLeftMargin(0.10);
  canvas->SetRightMargin(0.02);
  return canvas;
}

void draw_and_save(vector<shared_ptr<TH1F>> &hists,
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
  if(legend) legend->SetTextSize(0.03);
  canvas->SaveAs(path);
}
