#pragma once

#include <unordered_map>
#include <vector>
#include "hash_vector.h"

class BPatch_basicBlock;

namespace selfchecksum {

class checker;

using basic_blocks_collection = hash_vector<BPatch_basicBlock*>;
using checkers_collection = std::vector<checker>;
}

