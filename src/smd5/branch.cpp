#include "smd5/branch.h"
#include <TTree.h>
#include <TClonesArray.h>

using namespace std;

bool get_branch(TClonesArray *&array, TTree *tree, const char *name, const char *type)
{
  TBranch *branch = tree->GetBranch(name);
  if(branch == NULL) { array = NULL; return false; }

  array = new TClonesArray(type);
  if(array == NULL) return false;

  branch->SetAddress(&array);
  return true;
}
