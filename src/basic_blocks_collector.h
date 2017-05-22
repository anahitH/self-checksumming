#pragma once

#include "definitions.h"

class BPatch_binaryEdit;
class BPatch_basicBlock;
class BPatch_module;
class BPatch_function;

namespace selfchecksum {

class logger;

class basic_blocks_collector
{
public:
    basic_blocks_collector(BPatch_module* module, const logger& log);
    basic_blocks_collector(const modules_collection& modules, const logger& log);

    basic_blocks_collector(const basic_blocks_collector&) = delete;
    basic_blocks_collector& operator =(const basic_blocks_collector&) = delete;

public:
    const basic_blocks_collection& get_basic_blocks() const;
    basic_blocks_collection& get_basic_blocks();

    void collect();

    // debug function
    void dump() const;

private:
    void collect_from_function(BPatch_function* function);

private:
    modules_collection modules;
    const logger& log;
    basic_blocks_collection basic_blocks;
}; // class basic_blocks_collector

} // namespace selfchecksum

