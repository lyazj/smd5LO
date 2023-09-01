#include <classes/DelphesClasses.h>
#include <TFile.h>
#include <TTree.h>
#include <TClonesArray.h>
#include <vector>
#include <utility>
#include <string>
#include <memory>
#include <stdio.h>
#include <string.h>
#include <errno.h>

using namespace std;

int main(int argc, char *argv[])
{
  const char *rootfile;  // rootfile to be handled
  long long unitsize;  // maximum events per partition
  if(argc != 3) {
    fprintf(stderr, "usage: %s <rootfile> <unitsize>\n", program_invocation_short_name);
    return 1;
  }
  rootfile = argv[1];
  {
    size_t rootfile_len = strlen(rootfile);
    if(rootfile_len < 5 || strcmp(rootfile + (rootfile_len - 5), ".root")) {
      fprintf(stderr, "ERROR: invalid input file: %s\n", rootfile);
      return 1;
    }
  }
  unitsize = stoll(argv[2]);
  if(unitsize <= 0) {
    fprintf(stderr, "ERROR: invalid unit size: %lld\n", unitsize);
    return 1;
  }

  // Open rootfile to read.
  auto file = make_unique<TFile>(rootfile, "read");
  if(!file->IsOpen()) {
    fprintf(stderr, "ERROR: error opening file: %s\n", rootfile);
    return 1;
  }
  auto delphes = (TTree *)file->Get("Delphes");
  Long64_t n = delphes->GetEntries();
  printf("%s: %llu events total\n", rootfile, (unsigned long long)n);

  // Set up branches of interest.
  vector<pair<const char *, TClonesArray *>> branches;
  for(const auto &name_type : {
    make_pair("Jet", "Jet"),
    make_pair("FatJet", "Jet"),
    make_pair("Particle", "GenParticle"),
    make_pair("Track", "Track"),
    make_pair("Tower", "Tower"),
    make_pair("Muon", "Muon"),
    make_pair("ParticleFlowCandidate", "ParticleFlowCandidate"),
  }) {
    const char *name = name_type.first, *type = name_type.second;
    TBranch *branch = delphes->GetBranch(name);
    if(branch == NULL) continue;
    branches.emplace_back(name, new TClonesArray(type));
    branch->SetAddress(&branches.back().second);
  }

  // These force the linker to link libDelphes.so.
  Jet();
  GenParticle();
  Track();
  Tower();
  Muon();
  ParticleFlowCandidate();

  // Loop over entry partitions.
  size_t part_num = 0;
  for(Long64_t i = 0; i < n; i += unitsize) {
    // Determine partition name.
    char part_buf[64];
    sprintf(part_buf, "%06zu", part_num + 1);
    string outname = basename(rootfile);
    outname.resize(outname.size() - 5);
    outname += "_"s + part_buf + ".root";

    // Open partition file to write.
    auto outfile = make_unique<TFile>(outname.c_str(), "recreate");
    if(!file->IsOpen()) {
      fprintf(stderr, "ERROR: error opening file: %s\n", rootfile);
      return 1;
    }

    // Create output TTree object.
    auto tree = make_shared<TTree>(delphes->GetName(), delphes->GetTitle());
    for(auto &name_addr : branches) tree->Branch(name_addr.first, &name_addr.second);

    // Loop over entries.
    for(long long j = 0; j < unitsize; ++j) {
      if(i + j >= n) break;
      delphes->GetEntry(i + j);
      tree->Fill();
    }

    // Save results.
    tree->Write();
    printf("partition %zu: %s\n", part_num, outname.c_str());
    ++part_num;
  }

  // Clean up.
  for(auto &name_addr : branches) delete name_addr.second;
  return 0;
}
