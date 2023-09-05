#include "smd5/reco.h"
#include <TLorentzVector.h>

using namespace std;

Double_t higgs_reco1(const vector<TLorentzVector> &pb, const vector<int> &qb, vector<TLorentzVector> &ph)
{
  int nb = pb.size();
  ph = { };
  Double_t drh = INFINITY;

  // Higgs reconstruction 1/1: Which 2 b-jets to use.
  for(int b0 =  0; b0 <= nb - 2; ++b0) {
  for(int b1 = b0; b1 <= nb - 1; ++b1) {

    vector<TLorentzVector> ph_g(1);
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

Double_t higgs_reco2(const vector<TLorentzVector> &pb, const vector<int> &qb, vector<TLorentzVector> &ph)
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

    vector<TLorentzVector> ph_g(2);
    Double_t drh_g = 0.0;
    int ih = 0;
    for(const auto &i01 : g) {
      int i0 = i01.first, i1 = i01.second;
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
