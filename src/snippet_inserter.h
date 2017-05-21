#include "logger.h"

class BPatch_binaryEdit;
class BPatch_image;
class BPatch_basicBlock;


namespace selfchecksum {

class snippet_inserter
{
public:
    snippet_inserter(const std::string& binary_name,
                     BPatch_binaryEdit* binary,
                     const logger& log);
    ~snippet_inserter();

public:
    void insertAddrHash(BPatch_basicBlock *checker, BPatch_basicBlock *target, bool check_target_guard = true);

public:
// testing
    void insert();

private:
    //void analise_function(BPatch_function* function);

private:
    const std::string& binary_name;
    BPatch_binaryEdit* binary;
    const logger& log;
    BPatch_image* appImage;
    unsigned block_order;
};

}

