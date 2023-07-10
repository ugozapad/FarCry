project(CryFont)

add_definitions(-D_USRDLL -DCRYFONT_EXPORTS)

set(SRC_FILES
    "CryFont.cpp"
    "CryPakIO.cpp"
    "FFont.cpp"
    "FFontXML.cpp"
    "FontRenderer.cpp"
    "FontTexture.cpp"
    "FreeType2/src/autohint/autohint.c"
    "FreeType2/src/base/ftbase.c"
    "FreeType2/src/base/ftbbox.c"
    "FreeType2/src/base/ftbdf.c"
    "FreeType2/src/base/ftdebug.c"
    "FreeType2/src/base/ftglyph.c"
    "FreeType2/src/base/ftinit.c"
    "FreeType2/src/base/ftmm.c"
    "FreeType2/src/base/ftpfr.c"
    "FreeType2/src/base/ftsystem.c"
    "FreeType2/src/base/ftwinfnt.c"
    "FreeType2/src/cache/ftcache.c"
    "FreeType2/src/psaux/psaux.c"
    "FreeType2/src/pshinter/pshinter.c"
    "FreeType2/src/psnames/psnames.c"
    "FreeType2/src/raster/raster.c"
    "FreeType2/src/sfnt/sfnt.c"
    "FreeType2/src/smooth/smooth.c"
    "FreeType2/src/truetype/truetype.c"
    "FreeType2/src/winfonts/winfnt.c"
    "GlyphBitmap.cpp"
    "GlyphCache.cpp"
    "ICryFont.cpp"
    "StdAfx.cpp"
)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/SourceCode/CryCommon
    ${CMAKE_SOURCE_DIR}/SourceCode/CrytekDLL
    ${CMAKE_CURRENT_SOURCE_DIR}/FreeType2/include
)

target_link_libraries(${PROJECT_NAME})
