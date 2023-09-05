#pragma once

#include <string>
#include <vector>
#include <memory>

class TH1F;
class TCanvas;

// Set TH1 style.
TH1F *format(TH1F *th1f);

// Create histograms of one figure.
std::vector<std::shared_ptr<TH1F>> create_hists(const std::vector<std::string> &labels,
    int nbin, float xmin, float xmax);

// Create canvas to draw histograms.
std::shared_ptr<TCanvas> create_canvas();

// Draw and save histograms to a file.
void draw_and_save(const std::shared_ptr<TH1F> &hist,
    const char *path, const char *xtitle, const char *ytitle);
void draw_and_save(const std::vector<std::shared_ptr<TH1F>> &hists,
    const char *path, const char *xtitle, const char *ytitle);
