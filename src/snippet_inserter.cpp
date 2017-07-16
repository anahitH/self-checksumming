#include "snippet_inserter.h"
#include "logger.h"

#include "BPatch.h"
#include "BPatch_binaryEdit.h"
#include "BPatch_image.h"
#include "BPatch_point.h"
#include "BPatch_function.h"
#include "BPatch_flowGraph.h"
#include "Instruction.h"

#include <iostream>
#include <set>

namespace selfchecksum {

snippet_inserter::snippet_inserter(const std::string& b_name,
                                   BPatch_binaryEdit* b_edit,
                                   const logger& l)
    : binary_name(b_name)
    , binary(b_edit)
    , log(l)
    , block_order(0)
{
    appImage = binary->getImage();
}

snippet_inserter::~snippet_inserter() {
    if (binary == nullptr) {
        return;
    }
    const std::string new_name = binary_name + "_modified";
    if (!binary->writeFile(new_name.c_str())) {
        log.log_error("Failed to write file\n");
    }
    delete binary;
    binary = nullptr;
}

void snippet_inserter::insertBlockTag(BPatch_basicBlock *bb, unsigned long long block_id)
{
    unsigned long long tag = 0x11223344;
    tag = tag ^ (block_id << 4 * 8);
    BPatch_variableExpr *blockId = binary->malloc(*appImage->findType("unsigned long"));
    BPatch_arithExpr save(BPatch_assign, *blockId, BPatch_constExpr(tag));
    binary->insertSnippet(save, *bb->findEntryPoint());
}

void snippet_inserter::insertEndCheckTag(BPatch_basicBlock *bb, unsigned long long block_id)
{
    unsigned long long tag = 0x44332211;
    tag = tag ^ (block_id << 4 * 8);
    BPatch_variableExpr *blockId = binary->malloc(*appImage->findType("unsigned long"));
    BPatch_arithExpr save(BPatch_assign, *blockId, BPatch_constExpr(tag));
    binary->insertSnippet(save, *bb->findEntryPoint());
}


void snippet_inserter::insertAddrHash(BPatch_basicBlock *checker, 
                                      BPatch_basicBlock *target, 
                                      unsigned long long block_id,
                                      bool check_target_guard)
{
    unsigned long long tag = 0xabcdabcd;
    if (!check_target_guard)
        tag = tag ^ 0x8000000000000000;
    tag = tag ^ (block_id << 4 * 8);
    // Snippet seq for Adder hash.
    BPatch_Vector<BPatch_snippet*> hashSeq;
    // Create variables
    BPatch_variableExpr *hash = binary->malloc(*appImage->findType("unsigned long"));
    BPatch_variableExpr *expected = binary->malloc(*appImage->findType("unsigned long"));
    BPatch_variableExpr *startAddr = binary->malloc(*appImage->findType("unsigned long"));
    BPatch_variableExpr *endAddr = binary->malloc(*appImage->findType("unsigned long"));

    // Initialize variables
    hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *hash, BPatch_constExpr(0)));
    hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *expected, BPatch_constExpr(tag)));
    hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *startAddr, BPatch_constExpr(tag)));
    hashSeq.push_back(new BPatch_arithExpr(BPatch_assign, *endAddr, BPatch_constExpr(tag)));
    
    //TODO: add check for JMP in old target block?

    // while cond (startAddr < endAddr)
    BPatch_boolExpr whileCond(BPatch_lt, *startAddr, *endAddr);
    // While seq
    BPatch_Vector<BPatch_snippet*> whileSeq;
    // change constExpr to value at addr
    whileSeq.push_back(new BPatch_arithExpr(BPatch_assign, *hash, 
                                            BPatch_arithExpr(BPatch_plus, *hash, BPatch_arithExpr(BPatch_deref, *startAddr))));
    whileSeq.push_back(new BPatch_arithExpr(BPatch_assign,  *startAddr, 
                                            BPatch_arithExpr(BPatch_plus, *startAddr, BPatch_constExpr(1))));

    // Run hash while loop
    hashSeq.push_back(new BPatch_whileExpr(whileCond, BPatch_sequence(whileSeq)));
    // check if hash == expected, change nullExpr to response
    hashSeq.push_back(new BPatch_ifExpr(BPatch_boolExpr(BPatch_ne, *hash, *expected),
                                        BPatch_arithExpr(BPatch_assign, *hash, BPatch_constExpr(0x12345678))));

    binary->insertSnippet(BPatch_sequence(hashSeq), *checker->findEntryPoint());
}

}

