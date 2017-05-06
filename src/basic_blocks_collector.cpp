#include "basic_blocks_collector.h"

#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_point.h"

#include <set>
#include <vector>

namespace selfchecksum {

basic_blocks_collector::basic_blocks_collector(BPatch_binaryEdit& bin)
    : binary(bin)
{
}

const basic_blocks_collector::basic_blocks_collection& basic_blocks_collector::get_basic_blocks() const
{
    return basic_blocks;
}

void basic_blocks_collector::collect()
{
    BPatch_image* appImg = binary.getImage();
    std::vector<BPatch_module*> modules;
    appImg->getModules(modules);
    if (modules.empty()) {
        return;
    }
    unsigned index = 0;
    for (auto& module : modules) {
        collect_from_module(module, index);
    }

}

void basic_blocks_collector::collect_from_module(BPatch_module* module, unsigned& index)
{
    std::vector<BPatch_function*>* functions = module->getProcedures();
    if (functions == nullptr) {
        return;
    }
    for (auto& function : *functions) {
        collect_from_function(function, index);
    }
}

void basic_blocks_collector::collect_from_function(BPatch_function* function, unsigned& index)
{
    BPatch_flowGraph* cfg = function->getCFG();
    if (cfg == nullptr) {
        return;
    }
    std::set<BPatch_basicBlock*> blocks;
    cfg->getAllBasicBlocks(blocks);
    for (auto& bb : blocks) {
        basic_blocks[index++] = bb;
    }
}

void basic_blocks_collector::dump() const
{
    std::cout << "Total number of basic blocks " << basic_blocks.size() << "\n";
}

}

