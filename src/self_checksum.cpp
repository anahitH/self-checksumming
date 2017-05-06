#include <iostream>

#include "self_checksum.h"
#include "basic_blocks_collector.h"

#include "BPatch.h"

namespace selfchecksum {

void self_checksum::run(const std::string& binary_name, unsigned connectivity)
{
    BPatch bpatch;
    BPatch_binaryEdit* binary = bpatch.openBinary(binary_name.c_str(), true);
    selfchecksum::basic_blocks_collector collector(*binary);
    collector.collect();
    collector.dump();
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
    std::string binary_name("/home/anahitik/TUM_S17/SIP/Introspection/self-checksumming/tests/test");
    //unsigned connectivity = atoi(argv[1]);
    unsigned connectivity = 2;
    checksum.run(binary_name, connectivity);

    return 0;
}

