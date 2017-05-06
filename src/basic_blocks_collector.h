#pragma once

#include <unordered_map>

class BPatch_binaryEdit;
class BPatch_basicBlock;
class BPatch_module;
class BPatch_function;

namespace selfchecksum {

class basic_blocks_collector
{
private:
    using basic_blocks_collection = std::unordered_map<unsigned, BPatch_basicBlock*>;

public:
    basic_blocks_collector(BPatch_binaryEdit& bin);

    basic_blocks_collector(const basic_blocks_collector&) = delete;
    basic_blocks_collector& operator =(const basic_blocks_collector&) = delete;

public:
    const basic_blocks_collection& get_basic_blocks() const;

    void collect();

    // debug function
    void dump() const;

private:
    void collect_from_module(BPatch_module* module, unsigned& index);
    void collect_from_function(BPatch_function* function, unsigned& index);

private:
    BPatch_binaryEdit& binary;
    basic_blocks_collection basic_blocks;
}; // class basic_blocks_collector

} // namespace selfchecksum

