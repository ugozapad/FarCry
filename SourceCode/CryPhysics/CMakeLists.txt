project(CryPhysics)

add_definitions(-D_USRDLL -DPHYSICS_EXPORTS -DENTITY_PROFILER_ENABLED)
if (${ARCH_TYPE} STREQUAL "x86")
    add_definitions(-DPIII_SSE)
endif()

set(SRC_FILES
    "aabbtree.cpp"
    "articulatedentity.cpp"
    "boolean2d.cpp"
    "boxgeom.cpp"
    "CryPhysics.cpp"
    "cylindergeom.cpp"
    "geoman.cpp"
    "geometry.cpp"
    "heightfieldbv.cpp"
    "heightfieldgeom.cpp"
    "intersectionchecks.cpp"
    "linunprojectionchecks.cpp"
    "livingentity.cpp"
    "matrixnm.cpp"
    "obbtree.cpp"
    "overlapchecks.cpp"
    "particleentity.cpp"
    "physicalentity.cpp"
    "physicalplaceholder.cpp"
    "physicalworld.cpp"
    "qhull.cpp"
    "raybv.cpp"
    "raygeom.cpp"
    "rigidbody.cpp"
    "rigidentity.cpp"
    "ropeentity.cpp"
    "rotunprojectionchecks.cpp"
    "singleboxtree.cpp"
    "softentity.cpp"
    "spheregeom.cpp"
    "StdAfx.cpp"
    "trimesh.cpp"
    "utils.cpp"
    "wheeledvehicleentity.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_libraries(${PROJECT_NAME})
