#include <iostream>

#include "checkers_network.h"
#include "logger.h"
#include "self_checksum.h"

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
    checkers_network network(module, connectivity, log);
    network.build();
    network.dump();
}

}

int main(int argc, char* argv[])
{
    if (argc != 4) {
        std::cerr << "Wrong number of arguments\n";
        return 1;
    }
    selfchecksum::self_checksum checksum;
<<<<<<< HEAD
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
=======
    const std::string binary_name("/home/djwessel/Documents/sip/self-checksumming/build/testing/test");
    const std::string module_name("test");
    unsigned connectivity = 2;
>>>>>>> 539c9ae0c55db441a480bd0012202d4147717b1e
    checksum.run(binary_name, module_name, connectivity);

    return 0;
}

