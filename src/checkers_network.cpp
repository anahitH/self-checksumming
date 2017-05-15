#include "checkers_network.h"

#include "basic_blocks_collector.h"
#include "logger.h"

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_module.h"

#include <ctime>
#include <cstdlib>
#include <list>

namespace selfchecksum {

namespace {

basic_blocks_collection choose_checkers(BPatch_basicBlock* block, const basic_blocks_collection& remaining_blocks, unsigned conn_level)
{
    //const auto& dominators = get_dominator_blocks(block);
    return basic_blocks_collection();
}

std::unordered_set<acyclic_cfg::node_type> get_random_nodes(const hash_vector<acyclic_cfg::node_type>& dominators, unsigned number)
{
    srand(time(NULL));

    unsigned size = dominators.size();
    number = std::min(size, number);
    std::unordered_set<acyclic_cfg::node_type> nodes;
    for (unsigned i = 0; i < number;) {
        unsigned index = rand() % size;
        auto n = dominators.get(index);
        if (!nodes.insert(n).second) {
            continue;
        }
        ++i;
    }
    return nodes;
}

std::unordered_set<BPatch_basicBlock*> get_random_blocks(const hash_vector<BPatch_basicBlock*>& blocks, unsigned number)
{
    srand(time(NULL));

    unsigned size = blocks.size();
    number = std::min(size, number);
    std::unordered_set<BPatch_basicBlock*> random_blocks;
    for (unsigned i = 0; i < number;) {
        unsigned index = rand() % size;
        auto n = blocks.get(index);
        if (!random_blocks.insert(n).second) {
            continue;
        }
        ++i;
    }
    return random_blocks;
}

void detach_node_from_parents(acyclic_cfg::node_type block_node,
                              hash_vector<acyclic_cfg::node_type>& block_parents,
                              std::list<acyclic_cfg::node_type>& leaves)
{
    for (auto& parent : block_parents) {
        parent->remove_child(block_node);
        if (parent->is_leaf()) {
            leaves.push_front(parent);
        }
    }
}

void remove_random_blocks(const std::unordered_set<BPatch_basicBlock*>& checker_blocks,
                          hash_vector<BPatch_basicBlock*>& remaining_blocks)
{
    for (const auto& block : checker_blocks) {
        remaining_blocks.erase(block);
    }
}

}

BPatch_basicBlock* checkers_network::node::get_block()
{
    return basic_block;
}

void checkers_network::node::add_checker(node_type checker)
{
    block_checkers.push_back(checker);
}

void checkers_network::node::add_checkee(BPatch_basicBlock* block, bool check_checker)
{
    block_checkees.push_back(std::make_pair(block, check_checker));
}

checkers_network::node::node(BPatch_basicBlock* block)
    : basic_block(block)
{
}

checkers_network::checkers_network(BPatch_module* m, unsigned conn_level, const logger& l)
    : module(m)
    , connectivity_level(conn_level)
    , log(l)
{
}

void checkers_network::build()
{
    acyclic_call_graph call_graph(module);
    call_graph.build();

    logger log;
    basic_blocks_collector block_collector(*module, log);
    block_collector.collect();
    auto& all_blocks = block_collector.get_basic_blocks();

    auto& leaf_functions = call_graph.get_leaves();
    std::list<acyclic_call_graph::node_type> function_queue;
    std::unordered_set<acyclic_call_graph::node_type> processed_functions;
    for (auto& leaf_f : leaf_functions) {
        if (all_blocks.size() <= connectivity_level) {
            build_for_remaining_blocks(all_blocks);
            break;
        }
        function_queue.push_front(leaf_f);
        while (!function_queue.empty()) {
            auto function_node = function_queue.back();
            if (processed_functions.find(function_node) != processed_functions.end()) {
                continue;
            }
            function_queue.pop_back();
            function_node->build_cfg();
            auto& function_cfg = function_node->get_cfg();
            build(function_cfg, all_blocks);
            processed_functions.insert(function_node);

            const auto& callers = function_node->get_callers();
            function_queue.insert(function_queue.begin(), callers.begin(), callers.end());
        }
    }
}

void checkers_network::build(acyclic_cfg& function_cfg, basic_blocks_collection& remaining_blocks)
{
    std::list<acyclic_cfg::node_type> blocks_queue;
    auto& cfg_leaves = function_cfg.get_leaves();
    for (auto& leaf : cfg_leaves) {
        blocks_queue.push_front(leaf);
        while (!blocks_queue.empty()) {
            auto block_node = blocks_queue.back();
            auto res = network.insert(std::make_pair(block_node->get_block(), node_type(new node(block_node->get_block()))));
            auto check_node = res.first->second;
            if (res.second) { // newly added
                leaves.insert(check_node);
            }

            blocks_queue.pop_back();
            auto block_dominators = block_node->get_parents();
            auto checker_nodes = get_random_nodes(block_dominators, connectivity_level);
            add_dominator_checkers(check_node, checker_nodes);
            detach_node_from_parents(block_node, block_dominators, blocks_queue);
            if (checker_nodes.size() < connectivity_level) {
                unsigned num = connectivity_level - checker_nodes.size();
                auto checker_blocks = get_random_blocks(remaining_blocks, num);
                add_random_checkers(check_node, checker_blocks);
                remove_random_blocks(checker_blocks, remaining_blocks);
            }
        }
    }
}

void checkers_network::build_for_remaining_blocks(hash_vector<BPatch_basicBlock*>& all_blocks)
{
    for (auto& block : all_blocks) {
        std::unordered_set<BPatch_basicBlock*> checker_blocks(all_blocks.begin(), all_blocks.end());
        checker_blocks.erase(block);
        auto res = network.insert(std::make_pair(block, node_type(new node(block))));
        add_random_checkers(res.first->second, checker_blocks, false);
    }
    all_blocks.clear();
}

void checkers_network::add_dominator_checkers(node_type checkee_node,
                                              std::unordered_set<acyclic_cfg::node_type>& checkers)
{
    for (auto& checker : checkers) {
        auto res = network.insert(std::make_pair(checker->get_block(), node_type(new node(checker->get_block()))));
        auto checker_node = res.first->second;
        checkee_node->add_checker(checker_node);
        checker_node->add_checkee(checkee_node->get_block());
        leaves.erase(checker_node);
    }
}

void checkers_network::add_random_checkers(node_type checkee_node,
                                           std::unordered_set<BPatch_basicBlock*>& checker_blocks,
                                           bool check_checker)
{
    for (auto block : checker_blocks) {
        auto res = network.insert(std::make_pair(block, node_type(new node(block))));
        auto checker_node = res.first->second;
        checkee_node->add_checker(checker_node);
        checker_node->add_checkee(checkee_node->get_block(), check_checker);
        leaves.erase(checker_node);
    }
}


}

