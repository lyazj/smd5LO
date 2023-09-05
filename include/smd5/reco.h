#pragma once

#include <TDataType.h>
#include <vector>

class TLorentzVector;

/*
 * pb: 4-momenta of b quarks
 * qb: charges of b quarks
 * ph: reconstructed Higgs 4-momenta
 * @ret: sum of reconstruction Delta-Rs
 */
Double_t higgs_reco1(const std::vector<TLorentzVector> &pb, const std::vector<int> &qb, std::vector<TLorentzVector> &ph);
Double_t higgs_reco2(const std::vector<TLorentzVector> &pb, const std::vector<int> &qb, std::vector<TLorentzVector> &ph);
