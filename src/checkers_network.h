#pragma once

#include "acyclic_call_graph.h"
#include "acyclic_cfg.h"
#include "definitions.h"

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

class BPatch_basicBlock;
class BPatch_module;

namespace selfchecksum {

class logger;
class acyclic_cfg;

class checkers_network
{
public:
    class node;
    using node_type = std::shared_ptr<node>;

    class node
    {
    public:
        using checkee_data = std::pair<BPatch_basicBlock*, bool>;

     public:
        node() = default;
        node(BPatch_basicBlock* block);

        BPatch_basicBlock* get_block();
        void add_checker(node_type checker);
        void add_checkee(BPatch_basicBlock* block, bool check_checker = true);

    private:
        BPatch_basicBlock* basic_block;
        std::vector<checkee_data> block_checkees;
        std::vector<node_type> block_checkers;
    };

public:
    checkers_network(BPatch_module* module, unsigned connectivity_level, const logger& log);

public:
    void build();

private:
    void build(acyclic_cfg& function_cfg, basic_blocks_collection& remaining_blocks);
    void build_for_remaining_blocks(hash_vector<BPatch_basicBlock*>& all_blocks);
    void add_dominator_checkers(node_type checkee_node, std::unordered_set<acyclic_cfg::node_type>& checkers);
    void add_random_checkers(node_type checkee_node, std::unordered_set<BPatch_basicBlock*>& checker_blocks, bool check_checker = true);

private:
    BPatch_module* module;
    unsigned connectivity_level;
    const logger& log;
    std::unordered_set<node_type> leaves;
    std::unordered_map<BPatch_basicBlock*, node_type> network;
};

}

