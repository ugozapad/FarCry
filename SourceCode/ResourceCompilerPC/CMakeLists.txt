project(ResourceCompilerPC)

add_definitions(-D_USRDLL -DRESOURCECOMPILERPC_EXPORTS)

set(SRC_FILES
    "BoneLightBindInfo.cpp"
    "CgfConvertor.cpp"
    "CgfUtils.cpp"
    "ChunkFileReader.cpp"
    "CryBoneDesc.cpp"
    "CrySkinBase.cpp"
    "CrySkinBasisBuilder.cpp"
    "CrySkinBuilder.cpp"
    "CrySkinBuilderBase.cpp"
    "CrySkinFull.cpp"
    "CrySkinMorph.cpp"
    "CrySkinMorphBuilder.cpp"
    "CrySkinRigidBasis.cpp"
    "CryVertexBinding.cpp"
    "FileMapping.cpp"
    "GC_CgfConverter.cpp"
    "NvTriStrip/NvTriStrip.cpp"
    "NvTriStrip/NvTriStripObjects.cpp"
    "RenderMeshBuilder.cpp"
    "ResourceCompilerPC.cpp"
    "SkinDataSources.cpp"
    "SSEUtils.cpp"
    "StatCGFCompiler/CryStaticModel.cpp"
    "StatCGFCompiler/File.cpp"
    "StatCGFCompiler/Geom.cpp"
    "StatCGFCompiler/Helper.cpp"
    "StatCGFCompiler/Light.cpp"
    "StatCGFCompiler/Meshidx.cpp"
    "StatCGFCompiler/Node.cpp"
    "StatCGFCompiler/StatCGFCompiler.cpp"
    "StatCGFCompiler/StatCGFCompilerLB.cpp"
    "StatCGFCompiler/StatCGFCompilerLBSerialize.cpp"
    "StatCGFCompiler/StatCGFShadVol.cpp"
    "StatCGFCompiler/StatObjPhysics.cpp"
    "stdafx.cpp"
    "StencilShadowConnectivityBuilder.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/SourceCode/ResourceCompiler
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dbghelp.lib
)
