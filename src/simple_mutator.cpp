#include "BPatch.h"
#include "BPatch_function.h"
#include "BPatch_point.h"
#include "BPatch_snippet.h"


int main(int args, const char* argv[])
{
    BPatch bpatch;

    const char* path = argv[1];
    BPatch_binaryEdit* app = bpatch.openBinary(path);

    BPatch_image* appImage = app->getImage();

    std::vector<BPatch_function*> functions;
    appImage->findFunction("main", functions);
    if(functions.size() < 1) {
        fprintf(stderr, "Function not found\n");
        exit(1);
    }

    std::vector<BPatch_point*>* points = functions[0]->findPoint(BPatch_locEntry);

    BPatch_boolExpr condition(BPatch_le, BPatch_constExpr(1), BPatch_constExpr(0));
    std::vector<BPatch_snippet*> whileItems;
    BPatch_sequence while_sequence(whileItems);

    BPatch_whileExpr final_while(condition, while_sequence);

    BPatchSnippetHandle* handle = app->insertSnippet(final_while, *points);
    if(handle == nullptr) {
        std::cerr << "Something wrong with inserting snippet\n";
    }

    std::string newpath(path);
    newpath += "_instrumented";
    bool res = app->writeFile(newpath.c_str());
    if(!res) {
        std::cerr << "Writing file to disk failed.\n";
        return -1;
    }
    return 0;
}


