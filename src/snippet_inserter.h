#include "checkers_network.h"

class BPatch_binaryEdit;

namespace selfchecksum {

class snippet_inserter
{
public:
    snippet_inserter(BPatch_binaryEdit* binary);

public:
    void insert(BPatch_basicBlock* checker, const std::unordered_set<BPatch_basicBlock*>& checkees);

private:
    BPatch_binaryEdit* binary;
};

}

