#pragma once

#include "hash_vector.h"
#include "acyclic_cfg.h"

#include <memory>
#include <unordered_set>

class BPatch_module;
class BPatch_function;

namespace selfchecksum {

class acyclic_call_graph
{
public:
    class node;
    using node_type = std::shared_ptr<node>;

public:
    class node
    {
    public:
        node(BPatch_function* f);

        void build_cfg();
        const acyclic_cfg& get_cfg() const;
        acyclic_cfg& get_cfg();

        BPatch_function* get_function() const;
        void add_caller(node_type caller);
        const std::unordered_set<node_type>& get_callers() const;
        bool has_callee(BPatch_function* child) const;
        void add_callee(BPatch_function* child);

    private:
        BPatch_function* function;
        acyclic_cfg node_cfg;
        std::unordered_set<node_type> callers;
        std::unordered_set<BPatch_function*> callees;
    };

public:
    acyclic_call_graph(BPatch_module* m);

public:
    void build();

    void dump() const;

public:
    const std::unordered_set<node_type>& get_leaves() const;
    std::unordered_set<node_type>& get_leaves();

private:
    void dump(const node_type& n, unsigned level) const;

public:
    BPatch_module* module;
    std::unordered_set<node_type> leaves;
    std::unordered_map<BPatch_function*, node_type> function_nodes;
};

}
