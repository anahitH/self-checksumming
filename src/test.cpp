#include <iostream>

#include "BPatch.h"
#include "BPatch_basicBlock.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_flowGraph.h"
#include "BPatch_function.h"
#include "BPatch_image.h"
#include "BPatch_module.h"
#include "BPatch_point.h"

#include <set>
#include <unordered_map>

class binary_analiser
{
public:
    binary_analiser() = default;
    ~binary_analiser();

public:
    void analise(const std::string& binary_name);

    void dump() const;

private:
    void analise_module(BPatch_module* module);
    void analise_function(BPatch_function* function);

private:
    BPatch bpatch;
    std::string binary_name;
    BPatch_binaryEdit* binary;

    using basic_blocks = std::set<BPatch_basicBlock*>;
    std::unordered_map<BPatch_function*, basic_blocks> function_blocks;
};

binary_analiser::~binary_analiser()
{
    if (binary == nullptr) {
        return;
    }
    const std::string new_name = binary_name + "_modified";
    if (!binary->writeFile(new_name.c_str())) {
        std::cerr << "Failed to write file\n";
    }
    delete binary;
    binary = nullptr;
}

void binary_analiser::analise(const std::string& bin_name)
{
    binary_name = bin_name;
    binary = bpatch.openBinary(binary_name.c_str(), true);
    if (binary == nullptr) {
        std::cerr << "Open binary failed!\n";
        return;
    }

    BPatch_image* appImg = binary->getImage();
    std::vector<BPatch_module*> modules;
    appImg->getModules(modules);
    if (modules.empty()) {
        return;
    }
    for (auto& module : modules) {
        analise_module(module);
    }
}

void binary_analiser::dump() const
{
    for (const auto& fblocks : function_blocks) {
        std::cout << fblocks.first->getName() << "\n";
        for (const auto& block : fblocks.second) {
            std::cout << "  " << block->getBlockNumber() << "\n";
        }
    }
}
void binary_analiser::analise_module(BPatch_module* module)
{
    std::vector<BPatch_function*>* functions = module->getProcedures();
    if (functions == nullptr) {
        return;
    }
    for (auto& function : *functions) {
        analise_function(function);
    }
}

void binary_analiser::analise_function(BPatch_function* function)
{
    BPatch_flowGraph* cfg = function->getCFG();
    if (cfg == nullptr) {
        return;
    }
    cfg->getAllBasicBlocks(function_blocks[function]);
}

int main()
{
    binary_analiser analiser;
    analiser.analise("test");
    analiser.dump();
    return 0;
}

