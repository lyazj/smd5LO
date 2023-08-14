#pragma once

class TTree;
class TClonesArray;

bool get_branch(TClonesArray *&, TTree *, const char *name, const char *type);
