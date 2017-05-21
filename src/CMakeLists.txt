add_executable(self-checksumming
    basic_blocks_collector.cpp
    checkers_network.cpp
    logger.cpp
    self_checksum.cpp
    self_checksum.cpp
    acyclic_call_graph.cpp
    acyclic_cfg.cpp
    snippet_inserter.cpp
)

# Use C++11 to compile our pass (i.e., supply -std=c++11).
target_compile_features(self-checksumming PRIVATE cxx_range_for cxx_auto_type)

# LLVM is (typically) built with no C++ RTTI. We need to match that;
# otherwise, we'll get linker errors about missing RTTI data.
set_target_properties(self-checksumming PROPERTIES
    COMPILE_FLAGS "-ldyninstAPI -g"
)

target_link_libraries(self-checksumming "-ldyninstAPI")

