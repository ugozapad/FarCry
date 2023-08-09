project(CryAnimation LANGUAGES CXX ASM_MASM)

add_definitions(-DDO_ASM -DPROC_INTEL -D_USRDLL -DCRYANIMATION_EXPORTS)

set(SRC_FILES
    "AnimObject.cpp"
    "AnimObjectLoader.cpp"
    "AnimObjectManager.cpp"
    "BoneLightBindInfo.cpp"
    "BoneLightDynamicBind.cpp"
    "BSplineKnots.cpp"
    "BSplineOpen.cpp"
    "BSplineVec3dPacked.cpp"
    "CgfUtils.cpp"
    "ChunkFileReader.cpp"
    "Controller.cpp"
    "ControllerCryBone.cpp"
    "ControllerManager.cpp"
    "ControllerPackedBSpline.cpp"
    "ControllerTCB.cpp"
    "CryAnimationBase.cpp"
    "CryBone.cpp"
    "CryBoneDesc.cpp"
    "CryBoneHierarchyLoader.cpp"
    "CryBoneInfo.cpp"
    "CryCharBody.cpp"
    "CryCharDecal.cpp"
    "CryCharDecalBuilder.cpp"
    "CryCharDecalManager.cpp"
    "CryCharFxTrail.cpp"
    "CryCharInstance.cpp"
    "CryCharInstanceRenderParams.cpp"
    "CryCharManager.cpp"
    "CryCharParticleManager.cpp"
    "CryCharRenderElement.cpp"
    "CryCharReShadowManager.cpp"
    "CryCharReShadowVolume.cpp"
    "CryGeometryInfo.cpp"
    "CryGeomMorphTarget.cpp"
    "CryKeyInterpolation.cpp"
    "CryModEffAnimation.cpp"
    "CryModEffIKSolver.cpp"
    "CryModEffMorph.cpp"
    "CryModel-Data.cpp"
    "CryModel.cpp"
    "CryModelAnimationContainer.cpp"
    "CryModelArrays.cpp"
    "CryModelGeometryLoader.cpp"
    "CryModelLoader.cpp"
    "CryModelShadowVolume.cpp"
    "CryModelState.cpp"
    "CryModelStatePhys.cpp"
    "CryModelSubmesh.cpp"
    "CrySkinBase.cpp"
    "CrySkinBasisBuilder.cpp"
    "CrySkinBuilder.cpp"
    "CrySkinBuilderBase.cpp"
    "CrySkinFull.cpp"
    "CrySkinMorph.cpp"
    "CrySkinMorphBuilder.cpp"
    "CrySkinRigidBasis.cpp"
    "CryVertexBinding.cpp"
    "cvars.cpp"
    "DebugUtils.cpp"
    "drand.cpp"
    "FileMapping.cpp"
    "IncContHeap.cpp"
    "makepath.cpp"
    "MathUtils.cpp"
    "QuaternionExponentX87.cpp"
    "RenderUtils.cpp"
    "splitpath.cpp"
    "SSEUtils.cpp"
    "stdafx.cpp"
    "VertexBufferArrayDrivers.cpp"
    "XBoxExport.cpp"
)

if (${ARCH_TYPE} STREQUAL "x64")
    list(APPEND SRC_FILES "CrySkinAMD64.asm")
endif()

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_libraries(${PROJECT_NAME})
