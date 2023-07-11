project(CryScriptSystem)

add_definitions(-D_USRDLL -DCRYSCRIPTSYSTEM_EXPORTS)

set(SRC_FILES
    "CryScriptSystem.cpp"
    "FunctionHandler.cpp"
    "LuaCryPakIO.cpp"
    "LUA/lapi.c"
    "LUA/lcode.c"
    "LUA/ldebug.c"
    "LUA/ldo.c"
    "LUA/lfunc.c"
    "LUA/lgc.c"
    "LUA/lib/lauxlib.c"
    "LUA/lib/lbaselib.c"
    "LUA/lib/lbitlib.c"
    "LUA/lib/ldblib.c"
    "LUA/lib/liolib.c"
    "LUA/lib/lmathlib.c"
    "LUA/lib/lstrlib.c"
    "LUA/llex.c"
    "LUA/lmem.c"
    "LUA/lobject.c"
    "LUA/lopcodes.c"
    "LUA/lparser.c"
    "LUA/lstate.c"
    "LUA/lstring.c"
    "LUA/ltable.c"
    "LUA/ltests.c"
    "LUA/ltm.c"
    "LUA/lundump.c"
    "LUA/lvm.c"
    "LUA/lzio.c"
    "MemStats.cpp"
    "RecycleAllocator.cpp"
    "ScriptBuffer.cpp"
    "ScriptObject.cpp"
    "ScriptSystem.cpp"
    "StdAfx.cpp"
    "vectorlib.c"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_CURRENT_SOURCE_DIR}/LUA
)

target_link_libraries(${PROJECT_NAME})
