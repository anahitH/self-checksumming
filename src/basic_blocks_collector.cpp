#include "basic_blocks_collector.h"

#include "logger.h"

#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_point.h"

#include <set>
#include <vector>

namespace selfchecksum {

basic_blocks_collector::basic_blocks_collector(BPatch_module& m, const logger& l)
    : module(m)
    , log(l)
{
}

const basic_blocks_collection& basic_blocks_collector::get_basic_blocks() const
{
    return basic_blocks;
}

basic_blocks_collection& basic_blocks_collector::get_basic_blocks()
{
    return basic_blocks;
}


void basic_blocks_collector::collect()
{
    std::vector<BPatch_function*>* functions = module.getProcedures();
    if (functions == nullptr) {
        return;
    }
    for (auto& function : *functions) {
        collect_from_function(function);
    }
}

void basic_blocks_collector::collect_from_function(BPatch_function* function)
{
    BPatch_flowGraph* cfg = function->getCFG();
    if (cfg == nullptr) {
        return;
    }
    std::set<BPatch_basicBlock*> blocks;
    cfg->getAllBasicBlocks(blocks);
    for (auto& bb : blocks) {
        basic_blocks.push_back(bb);
    }
}

void basic_blocks_collector::dump() const
{
    std::string msg("Total number of basic blocks ");
    msg += std::to_string(basic_blocks.size());
    log.log_message(msg);
}

}

