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
    using network_type = std::unordered_map<BPatch_basicBlock*, node_type>;

    class node
    {
    public:
        using checkee_data = std::pair<BPatch_basicBlock*, bool>;
        using checkees_collection = std::unordered_map<BPatch_basicBlock*, bool>;
        using checkers_collection = std::vector<node_type>;

     public:
        node() = default;
        node(BPatch_basicBlock* block);

        BPatch_basicBlock* get_block();
        void add_checker(node_type checker);
        void add_checkee(BPatch_basicBlock* block, bool check_checker = true);

        void remove_checkee(BPatch_basicBlock* block);
        bool has_checkees() const;

        const checkees_collection& get_checkees() const;
        const checkers_collection& get_checkers() const;

        checkees_collection& get_checkees();
        checkers_collection& get_checkers();

        bool checks_only_block(BPatch_basicBlock* checkee) const;

    private:
        BPatch_basicBlock* basic_block;
        checkees_collection block_checkees;
        checkers_collection block_checkers;
    };

public:
    checkers_network(BPatch_module* module, unsigned connectivity_level, const logger& log);
    checkers_network(const modules_collection& module, unsigned connectivity_level, const logger& log);

public:
    const std::unordered_set<node_type>& get_leaves() const;
    std::unordered_set<node_type>& get_leaves();

public:
    void build();
    void dump(const std::string& network_name) const;

private:
    void build(acyclic_cfg& function_cfg, basic_blocks_collection& remaining_blocks);
    void build_for_remaining_blocks(hash_vector<BPatch_basicBlock*>& all_blocks);
    void add_dominator_checkers(node_type checkee_node, std::unordered_set<BPatch_basicBlock*>& checkers);
    void add_random_checkers(node_type checkee_node, std::unordered_set<BPatch_basicBlock*>& checker_blocks, bool check_checker = true);
    hash_vector<BPatch_basicBlock*> get_dominators(acyclic_cfg::node_type block_node);


private:
    modules_collection modules;
    unsigned connectivity_level;
    acyclic_call_graph call_graph;
    const logger& log;
    std::unordered_set<node_type> leaves;
    network_type network;
};

}

