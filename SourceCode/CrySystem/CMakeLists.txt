set(PROJECT_NAME CrySystem)

################################################################################
# Source groups
################################################################################
set(CheatProtection
    "DataProbe.cpp"
    "DataProbe.h"
    "md5.c"
    "md5.h"
    "RandGen.cpp"
    "RandGen.h"
)
source_group("CheatProtection" FILES ${CheatProtection})

set(HTTPDownloader
    "DownloadManager.cpp"
    "DownloadManager.h"
    "HTTPDownloader.cpp"
    "HTTPDownloader.h"
)
source_group("HTTPDownloader" FILES ${HTTPDownloader})

set(Header_Files
    "ApplicationHelper.h"
    "CPUDetect.h"
    "CryPak.h"
    "CrySizerImpl.h"
    "CrySizerStats.h"
    "CryWaterMark.h"
    "DebugCallStack.h"
    "FrameProfileSystem.h"
    "Log.h"
    "Mailer.h"
    "PakVars.h"
    "resource.h"
    "ScriptSink.h"
    "SourceSafeHelper.h"
    "StdAfx.h"
    "System.h"
    "SystemCFG.h"
    "Timer.h"
    "Validator.h"
    "XConsole.h"
    "XConsoleVariable.h"
)
source_group("Header Files" FILES ${Header_Files})

set(LuaDebugger
    "LuaDebugger/_TinyBrowseFolder.h"
    "LuaDebugger/_TinyCaptionWindow.h"
    "LuaDebugger/_TinyCoolEdit.h"
    "LuaDebugger/_TinyFileEnum.h"
    "LuaDebugger/_TinyImageList.h"
    "LuaDebugger/_TinyMain.h"
    "LuaDebugger/_TinyRegistry.h"
    "LuaDebugger/_TinySplitter.h"
    "LuaDebugger/_TinyStatusBar.h"
    "LuaDebugger/_TinyTreeList.h"
    "LuaDebugger/_TinyWindow.h"
    "LuaDebugger/AboutWnd.h"
    "LuaDebugger/FileTree.cpp"
    "LuaDebugger/FileTree.h"
    "LuaDebugger/LUADBG.cpp"
    "LuaDebugger/LUADBG.h"
    "LuaDebugger/LuaDbgInterface.cpp"
    "LuaDebugger/LuaDbgInterface.h"
)
source_group("LuaDebugger" FILES ${LuaDebugger})

set(MemoryManager
    "CryMemoryManager.cpp"
    "MTSafeAllocator.cpp"
    "MTSafeAllocator.h"
)
source_group("MemoryManager" FILES ${MemoryManager})

set(ScriptObjects
    "ScriptObjectAnimation.cpp"
    "ScriptObjectAnimation.h"
    "ScriptObjectEntity.cpp"
    "ScriptObjectEntity.h"
    "ScriptObjectMovie.cpp"
    "ScriptObjectMovie.h"
    "ScriptObjectParticle.cpp"
    "ScriptObjectParticle.h"
    "ScriptObjectScript.cpp"
    "ScriptObjectScript.h"
    "ScriptObjectSound.cpp"
    "ScriptObjectSound.h"
    "ScriptObjectSystem.cpp"
    "ScriptObjectSystem.h"
)
source_group("ScriptObjects" FILES ${ScriptObjects})

set(Source_Files
    "ApplicationHelper.cpp"
    "CompressedFile.cpp"
    "CPUDetect.cpp"
    "CryPak.cpp"
    "CrySizerImpl.cpp"
    "CrySizerStats.cpp"
    "DebugCallStack.cpp"
    "DllMain.cpp"
    "DxDiag.h"
    "FrameProfileRender.cpp"
    "FrameProfileSystem.cpp"
    "getdxver.cpp"
    "Log.cpp"
    "Mailer.cpp"
    "ScriptBinding.cpp"
    "ScriptSink.cpp"
    "SourceSafeHelper.cpp"
    "StdAfx.cpp"
    "System.cpp"
    "SystemCFG.cpp"
    "SystemInit.cpp"
    "SystemRender.cpp"
    "SystemWin32.cpp"
    "Timer.cpp"
    "XConsole.cpp"
    "XConsoleVariable.cpp"
)
source_group("Source Files" FILES ${Source_Files})

set(StreamEngine
    "../CryCommon/IStreamEngine.h"
    "CritSection.h"
    "RefReadStream.cpp"
    "RefReadStream.h"
    "RefReadStreamProxy.cpp"
    "RefReadStreamProxy.h"
    "RefStreamEngine.cpp"
    "RefStreamEngine.h"
    "StreamEngine.cpp"
    "StreamEngine.h"
)
source_group("StreamEngine" FILES ${StreamEngine})

set(XML
    "XML/_XMLDOMParserImpl.cpp"
    "XML/_XMLDOMParserImpl.h"
    "XML/nanoxml.h"
    "XML/xml.cpp"
    "XML/xml.h"
    "XML/xml_string.h"
    "XML/XMLDOMDocumentImpl.cpp"
    "XML/XMLDOMDocumentImpl.h"
    "XML/XMLDOMNodeImpl.cpp"
    "XML/XMLDOMNodeImpl.h"
    "XML/XMLDOMNodeListImpl.cpp"
    "XML/XMLDOMNodeListImpl.h"
)
source_group("XML" FILES ${XML})

set(XML__Expat
    "XML/Expat/ascii.h"
    "XML/Expat/asciitab.h"
    "XML/Expat/config.h"
    "XML/Expat/expat.h"
    "XML/Expat/iasciitab.h"
    "XML/Expat/latin1tab.h"
    "XML/Expat/nametab.h"
    "XML/Expat/utf8tab.h"
    "XML/Expat/winconfig.h"
    "XML/Expat/xmlparse.c"
    "XML/Expat/xmlrole.c"
    "XML/Expat/xmlrole.h"
    "XML/Expat/xmltok.c"
    "XML/Expat/xmltok.h"
#    "XML/Expat/xmltok_impl.c"
    "XML/Expat/xmltok_impl.h"
#    "XML/Expat/xmltok_ns.c"
)
source_group("XML\\Expat" FILES ${XML__Expat})

set(ZipDir
    "ZipDir.h"
    "ZipDirCache.cpp"
    "ZipDirCache.h"
    "ZipDirCacheFactory.cpp"
    "ZipDirCacheFactory.h"
    "ZipDirCacheRW.cpp"
    "ZipDirCacheRW.h"
    "ZipDirFind.cpp"
    "ZipDirFind.h"
    "ZipDirFindRW.cpp"
    "ZipDirFindRW.h"
    "ZipDirList.cpp"
    "ZipDirList.h"
    "ZipDirStructures.cpp"
    "ZipDirStructures.h"
    "ZipDirTree.cpp"
    "ZipDirTree.h"
    "ZipFile.h"
    "ZipFileFormat.h"
)
source_group("ZipDir" FILES ${ZipDir})

set(zlib
    "zlib/adler32.c"
    "zlib/compress.c"
    "zlib/crc32.c"
    "zlib/deflate.c"
    "zlib/deflate.h"
    "zlib/gzio.c"
    "zlib/infblock.c"
    "zlib/infblock.h"
    "zlib/infcodes.c"
    "zlib/infcodes.h"
    "zlib/inffast.c"
    "zlib/inffast.h"
    "zlib/inffixed.h"
    "zlib/inflate.c"
    "zlib/inftrees.c"
    "zlib/inftrees.h"
    "zlib/infutil.c"
    "zlib/infutil.h"
    "zlib/trees.c"
    "zlib/trees.h"
    "zlib/uncompr.c"
    "zlib/zconf.h"
    "zlib/zlib.h"
    "zlib/zutil.c"
    "zlib/zutil.h"
)
source_group("zlib" FILES ${zlib})

set(ALL_FILES
    ${CheatProtection}
    ${HTTPDownloader}
    ${Header_Files}
    ${LuaDebugger}
    ${MemoryManager}
    ${ScriptObjects}
    ${Source_Files}
    ${StreamEngine}
    ${XML}
    ${XML__Expat}
    ${ZipDir}
    ${zlib}
)

################################################################################
# Target
################################################################################
add_library(${PROJECT_NAME} SHARED ${ALL_FILES})

target_precompile_headers(${PROJECT_NAME} PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/StdAfx.h>"
)

#use_props(${PROJECT_NAME} "${CMAKE_CONFIGURATION_TYPES}" "${DEFAULT_CXX_PROPS}")
set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_GLOBAL_KEYWORD "Win32Proj"
)
################################################################################
# Target name
################################################################################
set_target_properties(${PROJECT_NAME} PROPERTIES
    TARGET_NAME_DEBUG     "${TARGET_NAME}$ENV{TargetExt}"
    TARGET_NAME_RELEASE   "${TARGET_NAME}$ENV{TargetExt}"
    TARGET_NAME_RELEASE64 "${TARGET_NAME}$ENV{TargetExt}"
)

set_target_properties(${PROJECT_NAME} PROPERTIES
OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Bin32"
OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Bin32"
OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../Bin32"
OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../Bin32"
LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/../Bin32"
LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/../Bin32"
LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/../Bin32"
LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/../Bin32"
)

################################################################################
# MSVC runtime library
################################################################################
#get_property(MSVC_RUNTIME_LIBRARY_DEFAULT TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY)
#string(CONCAT "MSVC_RUNTIME_LIBRARY_STR"
#    $<$<CONFIG:Debug>:
#        MultiThreadedDLL
#    >
#    $<$<CONFIG:Debug64>:
#        MultiThreadedDebugDLL
#    >
#    $<$<CONFIG:Profile>:
#        MultiThreadedDLL
#    >
#    $<$<CONFIG:Release>:
#        MultiThreadedDLL
#    >
#    $<$<CONFIG:Release64>:
#        MultiThreadedDLL
#    >
#    $<$<NOT:$<OR:$<CONFIG:Debug>,$<CONFIG:Debug64>,$<CONFIG:Profile>,$<CONFIG:Release>,$<CONFIG:Release64>>>:${MSVC_RUNTIME_LIBRARY_DEFAULT}>
#)
#set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY ${MSVC_RUNTIME_LIBRARY_STR})

################################################################################
# Include directories
################################################################################
target_include_directories(${PROJECT_NAME} PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../CryCommon"
)

################################################################################
# Compile definitions
################################################################################
#target_compile_definitions(${PROJECT_NAME} PRIVATE
#    "$<$<CONFIG:Debug>:"
#        "_RELEASE;"
#        "NDEBUG"
#    ">"
#    "$<$<CONFIG:Debug64>:"
#        "_DEBUG"
#    ">"
#    "$<$<CONFIG:Profile>:"
#        "NDEBUG"
#    ">"
#    "$<$<CONFIG:Release>:"
#        "_RELEASE;"
#        "NDEBUG"
#    ">"
#    "$<$<CONFIG:Release64>:"
#        "_RELEASE;"
#        "NDEBUG"
#    ">"
#    "WIN32;"
#   "_WINDOWS;"
#    "_USRDLL;"
#    "CRYSYSTEM_EXPORTS;"
#    "_MBCS"
#)

target_compile_definitions(${PROJECT_NAME} PRIVATE "CRYSYSTEM_EXPORTS")

################################################################################
# Compile and link options
################################################################################

################################################################################
# Dependencies
################################################################################
set(ADDITIONAL_LIBRARY_DEPENDENCIES
    "wininet"
)
target_link_libraries(${PROJECT_NAME} PUBLIC "${ADDITIONAL_LIBRARY_DEPENDENCIES}")

