#pragma once

class TClonesArray;

// Find the last same-pid child for a GenParticle.
int find_last_child(TClonesArray *particles, int i);
