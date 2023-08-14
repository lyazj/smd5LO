#include "smd5/utils.h"
#include <iostream>
#include <errno.h>

using namespace std;

int main(int argc, char *argv[])
{
  if(argc != 2) {
    cerr << "usage: " << program_invocation_short_name << " <proc-dir>" << endl;
    return 1;
  }
  string basedir = argv[1];
  if(basedir.empty()) basedir = ".";
  if(basedir.back() != '/') basedir.push_back('/');

  for(const Mg5Run &mg5run : list_run(basedir)) {
    string lhepath = basedir + mg5run.path + "/Events/run_01/unweighted_events.root";
    string cmd = "ls " + lhepath;
    system(cmd.c_str());
  }
  return 0;
}
