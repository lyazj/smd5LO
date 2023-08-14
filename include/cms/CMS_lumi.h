#pragma once

#include <TPad.h>
#include <TLatex.h>
#include <TLine.h>
#include <TBox.h>
#include <TASImage.h>

// source: https://github.com/UHHAnalysis/ZprimeFullHadAnalysis/blob/master/CMS_lumi.h
// modified by lyazj@github.com for compilation use

//
// Global variables
//

extern TString cmsText;
extern float cmsTextFont;  // default is helvetic-bold

extern bool writeExtraText;
extern TString extraText;
extern float extraTextFont;  // default is helvetica-italics

// text sizes and text offsets with respect to the top frame
// in unit of the top margin size
extern float lumiTextSize;
extern float lumiTextOffset;
extern float cmsTextSize;
extern float cmsTextOffset;  // only used in outOfFrame version

extern float relPosX;
extern float relPosY;
extern float relExtraDY;

// ratio of "CMS" and extra text size
extern float extraOverCmsTextSize;

extern TString lumi_13TeV;
extern TString lumi_8TeV;
extern TString lumi_7TeV;
extern TString lumi_sqrtS;

extern bool drawLogo;

void CMS_lumi(TPad *pad, int iPeriod = 3, int iPosX = 10);
