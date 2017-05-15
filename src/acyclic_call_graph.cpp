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

acyclic_call_graph::acyclic_call_graph(BPatch_module* m)
    : module(m)
{
}

const std::unordered_set<acyclic_call_graph::node_type>& acyclic_call_graph::get_leaves() const
{
    return leaves;
}

std::unordered_set<acyclic_call_graph::node_type>& acyclic_call_graph::get_leaves()
{
    return leaves;
}

void acyclic_call_graph::build()
{
    std::vector<BPatch_function*>* functions_p = module->getProcedures();
    if (functions_p == nullptr) {
        return;
    }
    auto& functions = *functions_p;

    while (!functions.empty()) {
        auto function = functions.back();
        functions.pop_back();
        node_type f_node(new node(function));
        auto res = function_nodes.insert(std::make_pair(function, f_node));
        if (!res.second) {
            f_node = res.first->second;
        }

        BPatch_Vector<BPatch_point*> points;
        function->getCallPoints(points);
        if (points.empty()) {
            leaves.insert(f_node);
        }
        points.clear();
        function->getCallerPoints(points);
        for (const auto& callPoint : points) {
            BPatch_function* callF = callPoint->getFunction();
            if (f_node->has_callee(callF)) {
                // cycle
                continue;
            }
            auto pos = function_nodes.find(callF);
            node_type call_node;
            if (pos != function_nodes.end()) {
                call_node = pos->second;
            } else {
                call_node = node_type(new node(callF));
                function_nodes.insert(std::make_pair(callF, call_node));
            }
            f_node->add_caller(call_node);
            call_node->add_callee(function);
        }
    }
}

void acyclic_call_graph::dump() const
{
    unsigned level = 0;
    for (const auto& n : leaves) {
        dump(n, level);
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

