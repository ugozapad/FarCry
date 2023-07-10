project(CrySoundSystem)

add_definitions(-D_USRDLL -D_WINDOWS -DCRYSOUNDSYSTEM_EXPORTS)

set(SRC_FILES
    "ADPCMDecoder.cpp"
    "CrySoundSystem.cpp"
    "MusicPattern.cpp"
    "MusicPatternInstance.cpp"
    "MusicSystem.cpp"
    "OGGDecoder.cpp"
    "PCMDecoder.cpp"
    "RandGen.cpp"
    "Sound.cpp"
    "SoundBuffer.cpp"
    "SoundSystem.cpp"
    "SoundSystemCommon.cpp"
    "StdAfx.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_CURRENT_SOURCE_DIR}/OggVorbisInclude
)

target_link_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    crysound.lib
    ogg_static.lib
    vorbis_static.lib
    vorbisfile_static.lib
)
