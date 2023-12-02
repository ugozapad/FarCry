cmake_minimum_required(VERSION 3.12 FATAL_ERROR)
cmake_policy(VERSION 3.12)

project(farcry LANGUAGES CXX)

if (CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_TYPE "x64")
else (CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(ARCH_TYPE "x86")
endif()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message("ARCH_TYPE=${ARCH_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}/bin/${ARCH_TYPE}-${CMAKE_BUILD_TYPE})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}/bin/${ARCH_TYPE}-${CMAKE_BUILD_TYPE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG})

if (MSVC)
    if (${ARCH_TYPE} STREQUAL "x64")
        add_definitions(-DWIN64 -D_AMD64_ -DOS_WIN64 -DPROC_AMD64)
    endif()
    # Always generate PDBs
    add_compile_options(/Zi)
    add_link_options(/DEBUG)

    add_compile_options(/W3) # Warning level
else()
    add_definitions(-DLINUX -fpermissive)
    if (${ARCH_TYPE} STREQUAL "x64")
        add_definitions(-DLINUX64)
    endif()

    # Verbose output
    add_compile_options(-v)
    add_link_options(-v)

    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS} -O3")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -g")
endif()

add_subdirectory(SourceCode)
