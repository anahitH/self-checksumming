#pragma once

#include <unordered_map>
#include <vector>
#include "hash_vector.h"

class BPatch_basicBlock;
class BPatch_module;

namespace selfchecksum {

class checker;

using basic_blocks_collection = hash_vector<BPatch_basicBlock*>;
using modules_collection = std::vector<BPatch_module*>;
}

