#include "smd5/particle.h"
#include <TClonesArray.h>
#include <classes/DelphesClasses.h>
#include <math.h>

using namespace std;

int find_last_child(TClonesArray *particles, int i)
{
  GenParticle *particle = (GenParticle *)particles->At(i);
  Int_t pid = particle->PID;
  Int_t d1 = particle->D1, d2 = particle->D2;
  if(d1 < 0) return i;

  // Find the same-PID daughter with the largest pT if any.
  int d0 = -1;
  Double_t pt0 = -INFINITY;
  for(int d = d1; d <= d2; ++d) {
    GenParticle *daughter = (GenParticle *)particles->At(d);
    if(daughter->PID != pid) continue;
    if(daughter->PT > pt0) { d0 = d; pt0 = daughter->PT; }
  }
  if(d0 < 0) return i;

  // Recurs until the last child.
  return find_last_child(particles, d0);
}
