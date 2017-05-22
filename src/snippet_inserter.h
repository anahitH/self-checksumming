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
    void insertBlockTag(BPatch_basicBlock *bb, unsigned long long block_id);
    void insertEndCheckTag(BPatch_basicBlock *bb, unsigned long long block_id);
    void insertAddrHash(BPatch_basicBlock *checker, BPatch_basicBlock *target, unsigned long long block_id, bool check_target_guard = true);

private:
    const std::string& binary_name;
    BPatch_binaryEdit* binary;
    const logger& log;
    BPatch_image* appImage;
    unsigned block_order;
};

}

