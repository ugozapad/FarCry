project(ResourceCompiler)

set(SRC_FILES
    "CfgFile.cpp"
    "CgfUtils.cpp"
    "ChunkFileReader.cpp"
    "CmdLine.cpp"
    "Config.cpp"
    "CryBoneDesc.cpp"
    "CryBoneHierarchyLoader.cpp"
    "CryChunkedFile.cpp"
    "CryVertexBinding.cpp"
    "ExtensionManager.cpp"
    "FileMapping.cpp"
    "FileUtil.cpp"
    "NvTriStrip/NvTriStrip.cpp"
    "NvTriStrip/NvTriStripObjects.cpp"
    "ResourceCompiler.cpp"
    "stdafx.cpp"
)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dbghelp.lib
)
