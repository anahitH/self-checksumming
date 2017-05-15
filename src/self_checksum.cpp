#include <iostream>

#include "basic_blocks_collector.h"
#include "logger.h"
#include "self_checksum.h"

#include "acyclic_call_graph.h"
#include "acyclic_cfg.h"

#include "BPatch.h"

namespace selfchecksum {

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
    std::vector<BPatch_function*> functions = *module->getProcedures();
    for (const auto& f : functions) {
        acyclic_cfg cfg(f);
        cfg.build();
        cfg.dump();
        std::cout << "------------------\n";
    }

    //acyclic_call_graph call_graph;
    //call_graph.create(module);
    //call_graph.dump();

    //selfchecksum::basic_blocks_collector collector(*binary, module_name, log);
    //collector.collect();
    //collector.dump();
}

}

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Wrong number of arguments\n";
        return 1;
    }
    selfchecksum::self_checksum checksum;
    //std::string binary_name(argv[2]);
    const std::string binary_name("/home/anahitik/TUM_S17/SIP/Introspection/self-checksumming/tests/test");
    const std::string module_name("test");
    //unsigned connectivity = atoi(argv[1]);
    unsigned connectivity = 2;
    checksum.run(binary_name, module_name, connectivity);

    return 0;
}

