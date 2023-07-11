project(FarCry)

set(SRC_FILES
    "FarCry.rc"
    "Main.cpp"
)

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/Externals/SDL/include
)

target_link_libraries(${PROJECT_NAME} SDL2)
