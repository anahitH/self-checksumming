#pragma once

#include "hash_vector.h"

#include <memory>
#include <unordered_set>

class BPatch_basicBlock;
class BPatch_function;

namespace selfchecksum {

class acyclic_cfg
{
public:
    class node;
    using node_type = std::shared_ptr<node>;

public:
    class node
    {
    public:
        node(BPatch_basicBlock* b);

        BPatch_basicBlock* get_block() const;
        void add_child(const node_type& child);
        void add_parent(const node_type& parent);
        void remove_child(const node_type& child);
        void remove_parent(const node_type& parent);
        
        const hash_vector<node_type>& get_children() const;
        const hash_vector<node_type>& get_parents() const;

        bool is_leaf() const;

    private:
        BPatch_basicBlock* block;
        hash_vector<node_type> children;
        hash_vector<node_type> parents;
    }; // class node
    
public:
    acyclic_cfg(BPatch_function* function);


    BPatch_function* get_function();
    const std::unordered_set<node_type>& get_leaves() const;
    std::unordered_set<node_type>& get_leaves();

    bool is_built() const
    {
        return built;
    }

public:
    void build();
    void dump() const;

private:
    void dump(const node_type& block, std::unordered_set<node_type>& processed) const;

private:
    BPatch_function* function;
    bool built;
    std::unordered_set<node_type> leaves;
};

}

