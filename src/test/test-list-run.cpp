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

  for(const Mg5Run &mg5run : list_run(argv[1])) {
    cout << mg5run.path << endl;
    cout << "run/" << mg5run.host << "-" << mg5run.time << "." << mg5run.nano_time << "\t";
    cout << mg5run.cs << "\t" << mg5run.cs_err << "\t" << mg5run.cs_unit << "\t" << mg5run.nevt << endl;
  }
  return 0;
}
