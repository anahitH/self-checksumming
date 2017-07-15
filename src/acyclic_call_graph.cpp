#include "acyclic_call_graph.h"

#include "BPatch.h"
#include "BPatch_module.h"
#include "BPatch_basicBlock.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_point.h"

namespace selfchecksum {

acyclic_call_graph::node::node(BPatch_function* f)
    : function(f)
    , node_cfg(function)
{
}

void acyclic_call_graph::node::build_cfg()
{
    if (!node_cfg.is_built()) {
        node_cfg.build();
    }
}

const acyclic_cfg& acyclic_call_graph::node::get_cfg() const
{
    return node_cfg;
}

acyclic_cfg& acyclic_call_graph::node::get_cfg()
{
    return node_cfg;
}

BPatch_function* acyclic_call_graph::node::get_function() const
{
    return function;
}

void acyclic_call_graph::node::add_caller(node_type caller)
{
    callers.insert(caller);
}

const std::unordered_set<acyclic_call_graph::node_type>& acyclic_call_graph::node::get_callers() const
{
    return callers;
}

bool acyclic_call_graph::node::has_callee(BPatch_function* callee) const
{
    return callees.find(callee) != callees.end();
}

void acyclic_call_graph::node::add_callee(BPatch_function* callee)
{
    callees.insert(callee);
}

void acyclic_call_graph::node::remove_callee(BPatch_function* callee)
{
    callees.erase(callee);
}

bool acyclic_call_graph::node::is_leaf() const
{
    return callees.empty();
}

acyclic_call_graph::acyclic_call_graph(BPatch_module* m)
{
    modules.insert(m);
}

acyclic_call_graph::acyclic_call_graph(const modules_collection& m)
{
    modules.insert(m.begin(), m.end());
}

const std::unordered_set<acyclic_call_graph::node_type>& acyclic_call_graph::get_leaves() const
{
    return leaves;
}

std::unordered_set<acyclic_call_graph::node_type>& acyclic_call_graph::get_leaves()
{
    return leaves;
}

const acyclic_call_graph::node_type& acyclic_call_graph::get_function_node(BPatch_function* function) const
{
    auto pos = function_nodes.find(function);
    assert(pos != function_nodes.end());
    return pos->second;
}

acyclic_call_graph::node_type& acyclic_call_graph::get_function_node(BPatch_function* function)
{
    auto pos = function_nodes.find(function);
    assert(pos != function_nodes.end());
    return pos->second;
}

void acyclic_call_graph::build()
{
    for (const auto& module : modules) {
        build(module);
    }
}

void acyclic_call_graph::build(BPatch_module* module)
{
    std::vector<BPatch_function*>* functions_p = module->getProcedures();
    if (functions_p == nullptr) {
        return;
    }
    auto& functions = *functions_p;

    while (!functions.empty()) {
        auto function = functions.back();

        functions.pop_back();
        auto res = function_nodes.insert(std::make_pair(function, node_type(new node(function))));
        node_type f_node = res.first->second;

        BPatch_Vector<BPatch_point*> points;
        function->getCallPoints(points);
        if (points.empty()) {
            leaves.insert(f_node);
            continue;
        }
        for (auto call_point : points) {
            auto f = call_point->getCalledFunction();
            if (f == nullptr || f == function || modules.find(f->getModule()) == modules.end() || !f->isInstrumentable()) {
                continue;
            }
            auto res = function_nodes.insert(std::make_pair(f, node_type(new node(f))));
            node_type callee_node = res.first->second;
            if (callee_node->has_callee(function)) {
                //cycle
                continue;
            }
            f_node->add_callee(f);
            assert(leaves.find(f_node) == leaves.end());
            callee_node->add_caller(f_node);
        }
        if (f_node->is_leaf()) {
            leaves.insert(f_node);
        }
    }

}

void acyclic_call_graph::dump() const
{
    for (const auto& n : function_nodes) {
        std::cout << n.first->getName() << "\n";
        for (const auto& caller : n.second->get_callers()) {
            std::cout << "      " << caller->get_function()->getName() << "\n";
        }
    }
    std::cout << "Leaves:\n";
    for (const auto& leaf : leaves) {
        std::cout << leaf->get_function()->getName() << "\n";
    }
}

void acyclic_call_graph::dump(const node_type& n, unsigned level) const
{
    for (unsigned i = 0; i < level; ++i) {
        std::cout << "  ";
    }
    std::cout << n->get_function()->getName() << "\n";
    for (const auto& caller : n->get_callers()) {
        dump(caller, ++level);
    }
    std::cout << "----\n";
}

}

