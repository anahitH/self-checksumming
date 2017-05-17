cmake_minimum_required(VERSION 3.1)

project(self-checksumming) 

set(${DYNINST_LIBRARY} /usr/local/lib/)
set(${DYNINST_INCLUDE} /usr/local/include/)
link_directories(${DYNINST_LIBRARY})
include_directories(${DYNINST_INCLUDE})

add_subdirectory(src)  # Use your pass name here.

