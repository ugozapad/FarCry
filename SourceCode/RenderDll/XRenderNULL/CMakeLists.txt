project(XRenderNULL)

add_definitions(-DDO_ASM -DNULL_RENDERER)

set(SRC_FILES
    "../Common/3DUtils.cpp"
    "../Common/CRT.cpp"
    "../Common/EvalFuncs_C.cpp"
    "../Common/EvalFuncs_RE.cpp"
    "../Common/LeafBufferCreate.cpp"
    "../Common/LeafBufferRender.cpp"
    "../Common/LeafBufferSerialize.cpp"
    "../Common/LightMaterial.cpp"
    "../Common/Names.cpp"
    "../Common/NvTriStrip/NvTriStrip.cpp"
    "../Common/NvTriStrip/NvTriStripObjects.cpp"
    "../Common/pip_addons.cpp"
    "../Common/RendElements/CRE2DQuad.cpp"
    "../Common/RendElements/CREBeam.cpp"
    "../Common/RendElements/CREClientPoly.cpp"
    "../Common/RendElements/CREClientPoly2D.cpp"
    "../Common/RendElements/CREDummy.cpp"
    "../Common/RendElements/CREFlares.cpp"
    "../Common/RendElements/CREGlare.cpp"
    "../Common/RendElements/CREHDRProcess.cpp"
    "../Common/RendElements/CREOcean.cpp"
    "../Common/RendElements/CREOcLeaf.cpp"
    "../Common/RendElements/CREOclusionQuery.cpp"
    "../Common/RendElements/CREParticleSpray.cpp"
    "../Common/RendElements/CREPolyBlend.cpp"
    "../Common/RendElements/CREPolyMesh.cpp"
    "../Common/RendElements/CREPrefabGeom.cpp"
    "../Common/RendElements/CREScreenProcess.cpp"
    "../Common/RendElements/CREShadowMap.cpp"
    "../Common/RendElements/CRESky.cpp"
    "../Common/RendElements/CRETempMesh.cpp"
    "../Common/RendElements/CRETerrainSector.cpp"
    "../Common/RendElements/CRETriMeshAdditionalShadow.cpp"
    "../Common/RendElements/CRETriMeshShadow.cpp"
    "../Common/RendElements/FFT_SSE.cpp"
    "../Common/RendElements/RendElement.cpp"
    "../Common/Renderer.cpp"
    "../Common/ResFile.cpp"
    "../Common/Shaders/Parser.cpp"
    "../Common/Shaders/ShaderComponents.cpp"
    "../Common/Shaders/ShaderCore.cpp"
    "../Common/Shaders/ShaderParse.cpp"
    "../Common/Shaders/ShaderScript.cpp"
    "../Common/Shaders/ShaderTemplate.cpp"
    "../Common/Textures/Image/BmpImage.cpp"
    "../Common/Textures/Image/CImage.cpp"
    "../Common/Textures/Image/DDSImage.cpp"
    "../Common/Textures/Image/inv_cmap.cpp"
    "../Common/Textures/Image/JpgImage.cpp"
    "../Common/Textures/Image/PcxImage.cpp"
    "../Common/Textures/Image/Quantize.cpp"
    "../Common/Textures/Image/TgaImage.cpp"
    "../Common/Textures/TexMan.cpp"
    "../Common/Textures/TexManShadows.cpp"
    "../Common/Textures/TexManStreaming.cpp"
    "../RenderPCH.cpp"
    "NULL_Font.cpp"
    "NULL_Renderer.cpp"
    "NULL_RendPipeline.cpp"
    "NULL_REOcean.cpp"
    "NULL_RERender.cpp"
    "NULL_Shaders.cpp"
    "NULL_Shadows.cpp"
    "NULL_System.cpp"
    "NULL_Textures.cpp"
    "NULL_TexturesStreaming.cpp"
    "NULL_VertBuffer.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/SourceCode/RenderDll
)

target_link_directories(${PROJECT_NAME} PRIVATE
    Libraries
)

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:libcp.lib /NODEFAULTLIB:LIBC.lib")

target_link_libraries(${PROJECT_NAME} PRIVATE
    ijl15.lib
    nvDXTlib.lib
    legacy_stdio_definitions.lib
)
