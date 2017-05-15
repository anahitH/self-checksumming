add_executable(snippets
    #TODO: add source files
    test.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(snippets PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(snippets PROPERTIES
    COMPILE_FLAGS "-ldyninstAPI -linstructionAPI"
)

target_link_libraries(snippets "-ldyninstAPI -linstructionAPI")

