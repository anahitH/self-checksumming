#include "checkers_network.h"

#include "basic_blocks_collector.h"
#include "logger.h"

#include "BPatch.h"
#include "BPatch_addressSpace.h"
#include "BPatch_basicBlock.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_module.h"

#include <ctime>
#include <cstdlib>
#include <list>
#include <iostream>
#include <fstream>
#include <sstream>

namespace selfchecksum {

namespace {

basic_blocks_collection choose_checkers(BPatch_basicBlock* block, const basic_blocks_collection& remaining_blocks, unsigned conn_level)
{
    //const auto& dominators = get_dominator_blocks(block);
    return basic_blocks_collection();
}

std::unordered_set<BPatch_basicBlock*> get_random_nodes(const hash_vector<BPatch_basicBlock*>& dominators, unsigned number)
{
    std::unordered_set<BPatch_basicBlock*> nodes;
    if (dominators.empty()) {
        return nodes;
    }
    srand(time(NULL));

    unsigned size = dominators.size();
    number = std::min(size, number);
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

std::unordered_set<BPatch_basicBlock*> get_random_blocks(const hash_vector<BPatch_basicBlock*>& blocks,
                                                         const std::unordered_set<BPatch_basicBlock*>& except,
                                                         unsigned number)
{
    srand(time(NULL));

    unsigned size = blocks.size();
    number = std::min(size, number);
    std::unordered_set<BPatch_basicBlock*> random_blocks;
    for (unsigned i = 0; i < number;) {
        unsigned index = rand() % size;
        auto n = blocks.get(index);
        if (except.find(n) != except.end()) {
            continue;
        }
        if (!random_blocks.insert(n).second) {
            continue;
        }
        ++i;
    }
    return random_blocks;
}

void detach_node_from_parents(acyclic_cfg::node_type& block_node,
                              std::list<acyclic_cfg::node_type>& leaves)
{
    auto block_parents = block_node->get_parents();
    for (auto& parent : block_parents) {
        parent->remove_child(block_node);
        if (parent->is_leaf()) {
            leaves.push_front(parent);
        }
    }
}

}

BPatch_basicBlock* checkers_network::node::get_block()
{
    return basic_block;
}
unsigned long long checkers_network::node::get_order_id()
{
    return order_id;
}
void checkers_network::node::set_order_id(unsigned long long id)
{
    order_id = id;
}

void checkers_network::node::add_checker(node_type checker)
{
    block_checkers.push_back(checker);
}

void checkers_network::node::add_checkee(BPatch_basicBlock* block, bool check_checker)
{
    block_checkees[block] = check_checker;
}

void checkers_network::node::remove_checkee(BPatch_basicBlock* block)
{
    block_checkees.erase(block);
}

bool checkers_network::node::has_checkees() const
{
    return !block_checkees.empty();
}

const checkers_network::node::checkees_collection& checkers_network::node::get_checkees() const
{
    return block_checkees;
}

checkers_network::node::checkees_collection& checkers_network::node::get_checkees()
{
    return block_checkees;
}

const checkers_network::node::checkers_collection& checkers_network::node::get_checkers() const
{
    return block_checkers;
}

checkers_network::node::checkers_collection& checkers_network::node::get_checkers()
{
    return block_checkers;
}

bool checkers_network::node::checks_only_block(BPatch_basicBlock* checkee) const
{
    auto pos = block_checkees.find(checkee);
    if (pos == block_checkees.end()) {
        return false;
    }
    return pos->second;
}

checkers_network::node::node(BPatch_basicBlock* block)
    : basic_block(block)
{
}

checkers_network::checkers_network(BPatch_module* m, unsigned conn_level, const logger& l)
    : connectivity_level(conn_level)
    , call_graph(m)
    , log(l)
    , block_order(0)
{
    modules.push_back(m);
}

checkers_network::checkers_network(const modules_collection& module, unsigned conn_level, const logger& l)
    : modules(module)
    , connectivity_level(conn_level)
    , call_graph(modules)
    , log(l)
{
}

const std::unordered_set<checkers_network::node_type>& checkers_network::get_nodes() const
{
    return nodes;
}

std::unordered_set<checkers_network::node_type>& checkers_network::get_nodes()
{
    return nodes;
}

const std::unordered_set<checkers_network::node_type>& checkers_network::get_leaves() const
{
    return leaves;
}

std::unordered_set<checkers_network::node_type>& checkers_network::get_leaves()
{
    return leaves;
}

void checkers_network::build()
{
    call_graph.build();

    logger log;
    basic_blocks_collector block_collector(modules, log);
    block_collector.collect();
    block_collector.dump();
    auto& all_blocks = block_collector.get_basic_blocks();

    std::list<acyclic_call_graph::node_type> function_queue;
    std::unordered_set<acyclic_call_graph::node_type> processed_functions;
    auto& leaf_functions = call_graph.get_leaves();
    function_queue.insert(function_queue.begin(), leaf_functions.begin(), leaf_functions.end());
    while (!function_queue.empty()) {
        auto function_node = function_queue.back();
        function_queue.pop_back();
        if (processed_functions.find(function_node) != processed_functions.end()) {
            continue;
        }
        function_node->build_cfg();
        auto& function_cfg = function_node->get_cfg();
        build(function_cfg, all_blocks);
        processed_functions.insert(function_node);

        const auto& callers = function_node->get_callers();
        function_queue.insert(function_queue.begin(), callers.begin(), callers.end());
        if (all_blocks.size() - 1 <= connectivity_level) {
            build_for_remaining_blocks(all_blocks);
            break;
        }
    }
}

void checkers_network::build(acyclic_cfg& function_cfg, basic_blocks_collection& remaining_blocks)
{
    std::list<acyclic_cfg::node_type> blocks_queue;
    auto& cfg_leaves = function_cfg.get_leaves();
    blocks_queue.insert(blocks_queue.begin(), cfg_leaves.begin(), cfg_leaves.end());
    while (!blocks_queue.empty()) {
        if (remaining_blocks.size() - 1 <= connectivity_level) {
            break;
        }
        auto block_node = blocks_queue.back();
        auto res = network.insert(std::make_pair(block_node->get_block(), node_type(new node(block_node->get_block()))));
        auto check_node = res.first->second;
        if (res.second) { // newly added
            leaves.insert(check_node);
        }

        blocks_queue.pop_back();
        remaining_blocks.erase(block_node->get_block());
        auto block_dominators = get_dominators(block_node);
        auto checker_nodes = get_random_nodes(block_dominators, connectivity_level);
        add_dominator_checkers(check_node, checker_nodes);
        if (checker_nodes.size() < connectivity_level) {
            unsigned num = connectivity_level - checker_nodes.size();
            auto checker_blocks = get_random_blocks(remaining_blocks, checker_nodes, num);
            add_random_checkers(check_node, checker_blocks);
        }
        check_node->set_order_id(block_order++);
        nodes.insert(check_node);
        detach_node_from_parents(block_node, blocks_queue);
    }
}

void checkers_network::build_for_remaining_blocks(hash_vector<BPatch_basicBlock*>& all_blocks)
{
    for (auto& block : all_blocks) {
        std::unordered_set<BPatch_basicBlock*> checker_blocks(all_blocks.begin(), all_blocks.end());
        checker_blocks.erase(block);
        auto res = network.insert(std::make_pair(block, node_type(new node(block))));
        auto check_node = res.first->second;
        add_random_checkers(check_node, checker_blocks, false);
        check_node->set_order_id(block_order++);
        nodes.insert(check_node);
    }
    all_blocks.clear();
}

void checkers_network::add_dominator_checkers(node_type checkee_node,
                                              std::unordered_set<BPatch_basicBlock*>& checkers)
{
    for (auto& checker : checkers) {
        auto res = network.insert(std::make_pair(checker, node_type(new node(checker))));
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

hash_vector<BPatch_basicBlock*> checkers_network::get_dominators(acyclic_cfg::node_type block_node)
{
    srand(time(NULL));
    hash_vector<BPatch_basicBlock*> dominators;
    const auto& parents = block_node->get_parents();
    for (auto& parent : parents) {
        dominators.push_back(parent->get_block());
    }

    BPatch_basicBlock* block = block_node->get_block();
    BPatch_flowGraph* cfg = block->getFlowGraph();
    BPatch_function* function = cfg->getFunction();
    
    unsigned number_of_tries = rand() % 7;
    while (dominators.size() < 2 * connectivity_level && number_of_tries-- != 0) {
        auto function_node = call_graph.get_function_node(function);
        auto callers = function_node->get_callers();
        if (callers.empty()) {
            // no need to continue
            break;
        }
        for (auto& caller : callers) {
            caller->build_cfg();
            auto& caller_cfg = caller->get_cfg();
            auto leaves = caller_cfg.get_leaves();
            unsigned random_index = rand() % leaves.size();
            auto iter = leaves.begin();
            while (random_index-- != 0 && ++iter != leaves.end());
            assert(iter != leaves.end());
            auto random_leaf = *iter;
            dominators.push_back(random_leaf->get_block());
            auto random_parents = random_leaf->get_parents();
            for (const auto& random_p : random_parents) {
                dominators.push_back(random_p->get_block());
            }
            function = caller->get_function();
        }
    }
    return dominators;
}

namespace graph_dump {

class dot_graph_writer
{
public:
    using network_type = std::unordered_map<BPatch_basicBlock*, checkers_network::node_type>;
    using node_connections = checkers_network::node::checkees_collection;

public:
    dot_graph_writer(const network_type& network, const std::string& name, const logger& l);

    void write();

private:
    void write_header();
    void write_node(BPatch_basicBlock* block_node);
    void write_edges(BPatch_basicBlock* block_node, const node_connections& node_checkees);
    void write_footer();
    void finish();

private:
    std::string create_header() const;
    std::string create_network_label() const;
    std::string create_node_label(BPatch_basicBlock* node) const;
    std::string create_node_id(BPatch_basicBlock* block) const;
    std::string create_edge_label(const std::string& node1_label, const checkers_network::node::checkee_data& checkee) const;

private:
    const network_type& network;
    const std::string& network_name;
    const logger& log;
    std::ofstream graph_stream;
};

dot_graph_writer::dot_graph_writer(const network_type& net, const std::string& name, const logger& l)
    : network(net)
    , network_name(name)
    , log(l)
{
}

void dot_graph_writer::write()
{
    graph_stream.open(network_name + ".dot");
    if (!graph_stream.is_open()) {
        log.log_error("Can not open file for writing the graph\n");
        return;
    }
    write_header();
    for (const auto& node : network) {
        write_node(node.first);
        write_edges(node.first, node.second->get_checkees());
    }
    write_footer();
    finish();
}

void dot_graph_writer::write_header()
{
    const std::string& header_label = create_header();
    graph_stream << header_label << std::endl;
}

void dot_graph_writer::write_node(BPatch_basicBlock* block_node)
{
    const std::string& node_label = create_node_label(block_node);
    graph_stream << "\t" << node_label << std::endl;
}

void dot_graph_writer::write_edges(BPatch_basicBlock* block_node, const node_connections& node_checkees)
{
    const std::string& node_id = create_node_id(block_node);
    for (const auto& checkee : node_checkees) {
        const std::string& connection = create_edge_label(node_id, checkee);
        graph_stream << "\t" << connection << std::endl;
    }
}

void dot_graph_writer::write_footer()
{
    graph_stream << "}";
}

void dot_graph_writer::finish()
{
    graph_stream.close();
}

std::string dot_graph_writer::create_header() const
{
    std::ostringstream header;
    header << "digraph ";
    const auto& graph_label = create_network_label();
    header << graph_label;
    header << " {\n";
    header << "\t";
    header << "label=";
    header << graph_label;
    header << ";\n";
    return header.str();
}

std::string dot_graph_writer::create_network_label() const
{
    return std::string("\"checkers network \'" + network_name + "\'\"");
}

std::string dot_graph_writer::create_node_label(BPatch_basicBlock* node) const
{
    // nodeid [shape=record, label="function name  nodeid"];
    BPatch_flowGraph* fg = node->getFlowGraph();
    const std::string& function_name = fg->getFunction()->getName();

    std::ostringstream node_label;
    node_label << create_node_id(node);
    node_label << " [shape=record,label=\"{";
    node_label << function_name << " block " << node->getBlockNumber();
    node_label << "}\"";
    node_label << "];";
    return node_label.str();
}

std::string dot_graph_writer::create_node_id(BPatch_basicBlock* block) const
{
    std::ostringstream id;
    id << "block" << block->getBlockNumber();
    return id.str();
}

std::string dot_graph_writer::create_edge_label(const std::string& node1_label,
                                                const checkers_network::node::checkee_data& checkee) const
{
    std::ostringstream label;
    label << node1_label << " -> ";
    label << create_node_id(checkee.first);
    if (!checkee.second) {
        label << " [color=\"red\"];";
    }
    return label.str();
}

}

void checkers_network::dump(const std::string& network_name) const
{
    graph_dump::dot_graph_writer graph_writer(network, network_name, log);
    graph_writer.write();
}

}

