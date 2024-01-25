project(CryEntitySystem)

add_definitions(-D_USRDLL -DCRYENTITYDLL_EXPORTS)

set(SRC_FILES
    "CryEntityDLL.cpp"
    "Entity.cpp"
    "EntityCamera.cpp"
    "EntityCreation.cpp"
    "EntityIt.cpp"
    "EntityPesistence.cpp"
    "EntityRender.cpp"
    "EntitySystem.cpp"
    "IDGenerator.cpp"
    "LipSync.cpp"
    "RandomExprLoadSink.cpp"
    "StdAfx.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_libraries(${PROJECT_NAME})
