project(CryInput)

add_definitions(-D_USRDLL -DCRYINPUT_EXPORTS)

set(SRC_FILES
    "CryInput.cpp"
    "Input.cpp"
    "Joystick.cpp"
    "StdAfx.cpp"
    "XActionMap.cpp"
    "XActionMapManager.cpp"
    "XKeyboard.cpp"
    "XMouse.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/Externals/SDL/include
)

target_link_libraries(${PROJECT_NAME} SDL2)
