#pragma once

class TTree;
class TClonesArray;

// Shortcut for reading TTree branches with TClonesArray type.
bool get_branch(TClonesArray *&, TTree *, const char *name, const char *type);

// Shortcut for reading and dumping TTree branches with TClonesArray type.
bool get_branch(TClonesArray *&, TTree *, TTree *, const char *name, const char *type);
