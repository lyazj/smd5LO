#ifdef __CLING__
#include "../include/smd5/utils.h"
#include "../resource/MG5_aMC/Delphes/classes/DelphesClasses.h"
R__LOAD_LIBRARY(../lib/libsmd5.so)
R__LOAD_LIBRARY(../resource/MG5_aMC/Delphes/libDelphes.so)
#else  /* __CLING__ */
#include "smd5/utils.h"
#include <classes/DelphesClasses.h>
#include <TFile.h>
#include <TTree.h>
#include <iostream>
#include <errno.h>
#include <memory>
#endif  /* __CLING__ */

using namespace std;

void list_selected(const vector<string> &procdirs = { "../mc/hhmumu" });

int main(int argc, char *argv[])
{
  // Linking hack.
  GenParticle();

  // Get running directory from cmdline.
  if(argc < 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir> [ <proc-dir> ... ]" << endl;
    return 1;
  }
  list_selected({ &argv[1], &argv[argc] });
  return 0;
}

void list_selected(const vector<string> &procdirs)
{
  // Traverse process directories.
  for(string procdir : procdirs) {
    size_t cur, cnt = 0;
    if(procdir.empty()) procdir = ".";
    if(procdir.back() != '/') procdir.push_back('/');
    clog << "INFO: working directory: \"" << procdir << "\"" << endl;

    // Get running info and traverse the runs.
    vector<Mg5Run> mg5runs = list_run(procdir);
    if(mg5runs.empty()) {
      clog << "WARNING: directory without any result: \"" << procdir << "\"" << endl;
      continue;
    }
    for(const Mg5Run &mg5run : mg5runs) {
    for(const string &rootpath : mg5run.selected()) {
      string lhepath = procdir + rootpath;
      auto file = make_shared<TFile>(lhepath.c_str());
      if(!file->IsOpen()) {
        cerr << "ERROR: error opening file to read: " << lhepath << endl;
        continue;
      }

      // Get the Selected tree.
      auto Selected = (TTree *)file->Get("Selected");
      if(Selected == NULL) {
        cerr << "ERROR: error getting Selected tree: " << lhepath << endl;
        continue;
      }
      cnt += cur = Selected->GetEntries();
      cout << lhepath << ": " << cur << endl;
    } }
    cout << "total in \"" << procdir << "\": " << cnt << endl;

  }
}
