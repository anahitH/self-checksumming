add_executable(self-checksumming
    #TODO: add source files
    self_checksum.cpp
    basic_blocks_collector.cpp
    self_checksum.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(self-checksumming PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(self-checksumming PROPERTIES
    COMPILE_FLAGS "-ldyninstAPI"
)

target_link_libraries(self-checksumming "-ldyninstAPI")

