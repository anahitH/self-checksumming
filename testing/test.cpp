#include <iostream>
#include <set>

#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_image.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"
#include "Instruction.h"

class snippet_inserter
{
public:
    snippet_inserter() = default;
    ~snippet_inserter();

public:
    void insert(const std::string& binary_name);

private:
    //void analise_function(BPatch_function* function);

private:
    BPatch bpatch;
    std::string binary_name;
    BPatch_binaryEdit* binary;
};

snippet_inserter::~snippet_inserter() {
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

void snippet_inserter::insert(const std::string& bin_name)
{
    BPatch_image *appImage;
    BPatch_Vector<BPatch_point*> *points;
    BPatch_Vector<BPatch_function *> functions;
    unsigned long h = 0, start, end, buffer;
    unsigned char byte;
    FILE *bFile = fopen("test", "rb");

    binary_name = bin_name;
    binary = bpatch.openBinary(binary_name.c_str(), true);
    if (binary == nullptr) {
        std::cerr << "Open binary failed!\n";
        return;
    }

    // Open program Image
    appImage = binary->getImage();
    // Find and return the main function
    appImage->findFunction("_Z4testv", functions);
    // Find and return the entry point to main
    points = functions[0]->findPoint(BPatch_entry);

    BPatch_flowGraph* cfg = functions[0]->getCFG();
    std::vector<Dyninst::InstructionAPI::Instruction::Ptr> insts, insts2, insts3;
    std::set<BPatch_basicBlock*> blocks;
    cfg->getAllBasicBlocks(blocks);

    // Inst add hash
    for (auto& bb : blocks) {
        h = 0;
        bb->getInstructions(insts);
        for (auto& inst : insts) {
            for (int i = 0; i < inst->size(); i++) {
                byte = inst->rawByte(i);
                h += byte;
                printf("RawByte: %x\n", byte);
            }
        }
        printf("hash: %lx\n", h);

        // Snippet seq for Adder hash.
        BPatch_Vector<BPatch_snippet*> hashSeq;
        // Create variables
        BPatch_variableExpr *hash = binary->malloc(*appImage->findType("unsigned long"));
        BPatch_variableExpr *startAddr = binary->malloc(*appImage->findType("unsigned long"));
        BPatch_variableExpr *endAddr = binary->malloc(*appImage->findType("unsigned long"));

        // Initialize variables
        hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *hash, BPatch_constExpr(0)));
        hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *startAddr, BPatch_constExpr(bb->getStartAddress())));
        hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *endAddr, BPatch_constExpr(bb->getEndAddress())));
        // while cond (startAddr < endAddr)
        BPatch_boolExpr whileCond(BPatch_lt, *startAddr, *endAddr);
        // While seq
        BPatch_Vector<BPatch_snippet*> whileSeq;
        // change constExpr to value at addr
        whileSeq.push_back(new BPatch_arithExpr(BPatch_assign, 
          *hash, 
          BPatch_arithExpr(BPatch_plus, 
            *hash, 
            BPatch_arithExpr(BPatch_deref, *startAddr))));
        whileSeq.push_back(new BPatch_arithExpr(BPatch_assign, 
          *startAddr, 
          BPatch_arithExpr(BPatch_plus, *startAddr, BPatch_constExpr(8))));

        // Run hash while loop
        hashSeq.push_back(new BPatch_whileExpr(whileCond, BPatch_sequence(whileSeq)));
        // check if hash == expected, change nullExpr to response
        hashSeq.push_back(new BPatch_ifExpr(BPatch_boolExpr(BPatch_ne, *hash, BPatch_constExpr(h)), BPatch_nullExpr()));

        // Change to use correct points
        binary->insertSnippet(BPatch_sequence(hashSeq), *bb->findEntryPoint());
    }

    // Inst xor hash
    for (auto& bb : blocks) {
        h = 0;
        int j = 0;
        bb->getInstructions(insts2);
        for (auto& inst : insts2) {
            for (int i = 0; i < inst->size(); i++) {
                buffer = inst->rawByte(i);
                printf("buffer shift: %lx\n", buffer << (j%8) * 8);
                h ^= buffer << (j % 8) * 8;
                j += 1;
            }
        }
        printf("hash: %lx\n", h);
    }



    // Create a counter var
    //BPatch_variableExpr *intCounter = binary->malloc(*appImage->findType("int"));
    // Create arithmetic expression
    //BPatch_arithExpr addOne(BPatch_assign, *intCounter, BPatch_arithExpr(BPatch_plus, *intCounter, BPatch_constExpr(1)));
    // Insert into binary
    //binary->insertSnippet(addOne, *points);
}

int main()
{
    std::cout << "Insert\n";
    snippet_inserter inserter;
    inserter.insert("test");
    std::cout << "Finished\n";
    return 0;
}
