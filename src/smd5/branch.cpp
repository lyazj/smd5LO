#include "smd5/branch.h"
#include <TTree.h>
#include <TClonesArray.h>
#include <string.h>

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

bool get_branch(TClonesArray *&array, TTree *tree, TTree *dumptree, const char *name, const char *type)
{
  if(!get_branch(array, tree, name, type)) return false;

  if(strlen(dumptree->GetName()) == 0) {
    dumptree->SetName(tree->GetName());
  }
  if(strlen(dumptree->GetTitle()) == 0) {
    dumptree->SetTitle(tree->GetTitle());
  }

  TBranch *branch = dumptree->GetBranch(name);
  if(branch) {
    branch->SetAddress(&array);
  } else {
    branch = dumptree->Branch(name, &array);
  }

  return true;
}
