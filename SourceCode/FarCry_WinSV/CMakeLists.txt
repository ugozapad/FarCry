project(FarCry_WinSV)

set(SRC_FILES
    "BasicConsole.cpp"
    "DedicatedServer.cpp"
    "FarCry_WinSV.cpp"
    "FarCry_WinSV.rc"
    "stdafx.cpp"
    "WinAndConsole.cpp"
)

add_executable(${PROJECT_NAME} WIN32 ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
)

target_link_libraries(${PROJECT_NAME})
