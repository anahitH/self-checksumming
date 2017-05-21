#include "acyclic_cfg.h"

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_flowGraph.h"
#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_point.h"

#include <unordered_map>

namespace selfchecksum
{

acyclic_cfg::node::node(BPatch_basicBlock* b)
    : block (b)
{
}

BPatch_basicBlock* acyclic_cfg::node::get_block() const
{
    return block;
}

void acyclic_cfg::node::add_child(const node_type& child)
{
    children.push_back(child);
}

void acyclic_cfg::node::add_parent(const node_type& parent)
{
    parents.push_back(parent);
}

void acyclic_cfg::node::remove_child(const node_type& child)
{
    children.erase(child);
}

void acyclic_cfg::node::remove_parent(const node_type& parent)
{
    parents.erase(parent);
}

bool acyclic_cfg::node::is_leaf() const
{
    return children.size() == 0;
}

const hash_vector<acyclic_cfg::node_type>& acyclic_cfg::node::get_children() const
{
    return children;
}

const hash_vector<acyclic_cfg::node_type>& acyclic_cfg::node::get_parents() const
{
    return parents;
}

acyclic_cfg::acyclic_cfg(BPatch_function* f)
    : function(f)
    , built(false)
{
}

BPatch_function* acyclic_cfg::get_function()
{
    return function;
}

const std::unordered_set<acyclic_cfg::node_type>& acyclic_cfg::get_leaves() const
{
    return leaves;
}

std::unordered_set<acyclic_cfg::node_type>& acyclic_cfg::get_leaves()
{
    return leaves;
}

void acyclic_cfg::build()
{
    BPatch_flowGraph* cfg = function->getCFG();
    BPatch_Vector<BPatch_basicBlock*> entry_blocks;
    if (!cfg->getEntryBasicBlock(entry_blocks)) {
        // log message
        return;
    }
    assert(entry_blocks.size() == 1);
    std::vector<BPatch_basicBlock*> to_process;
    std::unordered_map<BPatch_basicBlock*, node_type> block_nodes;

    to_process.insert(to_process.begin(), entry_blocks.begin(), entry_blocks.end());
    while (!to_process.empty()) {
        BPatch_basicBlock* block = to_process.back();
        to_process.pop_back();
        node_type block_node(new node(block));
        auto res = block_nodes.insert(std::make_pair(block, block_node));
        if (!res.second) {
            block_node = res.first->second;
        } else {
            leaves.insert(block_node);
        }
        BPatch_Set<BPatch_basicBlock*> dominates;
        block->getAllDominates(dominates);
        // e.g. control flow with a single block.
        if (dominates.size() == 1) {
            assert(*dominates.begin() == block);
            continue;
        }
        for (auto& dom : dominates) {
            if (dom == block) {
                continue;
            }
            auto res = block_nodes.insert(std::make_pair(dom, node_type(new node(dom))));
            auto post_node = res.first->second;
            if (res.second) {
                post_node->add_parent(block_node);
                block_node->add_child(post_node);
                to_process.insert(to_process.begin(), dom);
                leaves.insert(post_node);
                leaves.erase(block_node);
            } else if (!dom->postdominates(block)) {
                post_node->add_parent(block_node);
                block_node->add_child(post_node);
                leaves.erase(block_node);
            }
        }
    }
    built = true;
}

void acyclic_cfg::dump() const
{
    std::cout << "Function: " << function->getName() << "\n";
    std::unordered_set<node_type> processed;
    for (const auto& block : leaves) {
        dump(block, processed);
        std::cout << "\n";
    }
}

void acyclic_cfg::dump(const node_type& block, std::unordered_set<node_type>& processed) const
{
    if (processed.find(block) != processed.end()) {
        return;
    }
    processed.insert(block);
    std::cout << block->get_block()->getBlockNumber() << "   ";
    for (const auto& child : block->get_children()) {
        dump(child, processed);
    }
}
}

