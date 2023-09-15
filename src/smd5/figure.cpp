#include "smd5/figure.h"
#include <TH1F.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <string.h>

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

static void draw(TH1F *hist, const char *xtitle, const char *ytitle)
{
  hist->SetXTitle(xtitle);
  hist->SetYTitle(ytitle);
  format(hist)->DrawNormalized();
}

static void save(TCanvas *canvas, const char *path)
{
  char *const str = strdup(path);
  const char *const delim = ":";
  char *saveptr;
  char *token = strtok_r(str, delim, &saveptr);
  while(token) {
    canvas->SaveAs(token);
    token = strtok_r(NULL, delim, &saveptr);
  }
  free(str);
}

void draw_and_save(const shared_ptr<TH1F> &hist, const char *path,
    const char *xtitle, const char *ytitle, function<void(TCanvas *)> pp)
{
  auto canvas = create_canvas();
  draw(hist.get(), xtitle, ytitle);
  if(pp) pp(canvas.get());
  save(canvas.get(), path);
}

void draw_and_save(const vector<shared_ptr<TH1F>> &hists, const char *path,
    const char *xtitle, const char *ytitle, function<void(TCanvas *)> pp)
{
  auto canvas = create_canvas();
  for(size_t i = 0; i < hists.size(); ++i) {
    TH1F *hist = hists[i].get();
    if(hist->GetEntries() == 0) continue;
    hist->SetLineColor(i + 2);
    draw(hist, xtitle, ytitle);
  }
  TLegend *legend = canvas->BuildLegend(0.6, 0.92, 0.95, 0.8);
  if(legend) legend->SetTextSize(0.03);
  if(pp) pp(canvas.get());
  save(canvas.get(), path);
}
