#include <iostream>

#include "checkers_network.h"
#include "logger.h"
#include "self_checksum.h"
#include "snippet_inserter.h"

#include "BPatch.h"

#include <list>

namespace selfchecksum {

namespace {
void insert_snippets(const std::string& binary_name,
                     BPatch_binaryEdit* binary,
                     const logger& log,
                     checkers_network& network)
{
    snippet_inserter inserter(binary_name, binary, log);

    auto& leaves = network.get_leaves();
    std::list<checkers_network::node_type> checkers_queue;
    checkers_queue.insert(checkers_queue.begin(), leaves.begin(), leaves.end());

    while (!checkers_queue.empty()) {
        auto leaf = checkers_queue.back();
        checkers_queue.pop_back();
        BPatch_basicBlock* leaf_block = leaf->get_block();
        auto& checkers = leaf->get_checkers();
        for (auto& checker : checkers) {
            inserter.insertAddrHash(checker->get_block(), leaf_block, checker->checks_only_block(leaf_block));
            checker->remove_checkee(leaf_block);
            if (!checker->has_checkees()) {
                checkers_queue.push_front(checker);
            }
        }
    }
}

}

void self_checksum::run(const std::string& binary_name, const std::string& module_name, unsigned connectivity)
{
    logger log;
    BPatch bpatch;
    BPatch_binaryEdit* binary = bpatch.openBinary(binary_name.c_str(), true);
    BPatch_image* appImg = binary->getImage();
    if (appImg == nullptr) {
        return;
    }
    BPatch_module* module = appImg->findModule(module_name.c_str(), false);
    if (module == nullptr) {
        return;
    }
    checkers_network network(module, connectivity, log);
    network.build();
    //network.dump();

    insert_snippets(binary_name, binary, log, network);
}

}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Wrong number of arguments\n";
        return 1;
    }
    selfchecksum::self_checksum checksum;
    std::string binary_name(argv[1]);
    std::string module_name(argv[2]);
    unsigned connectivity = atoi(argv[3]);
    //std::string mesage("Creating network graph for executable: " + binary_name
    //                + ", module: " + module_name
    //                + ", connectivity level: " + std::to_string(connectivity));

    //std::cout << mesage << "\n";
    //const std::string binary_name("/home/anahitik/TUM_S17/SIP/Introspection/self-checksumming/tests/test");
    //const std::string module_name("test");
    //unsigned connectivity = 2;
    checksum.run(binary_name, module_name, connectivity);

    return 0;
}

