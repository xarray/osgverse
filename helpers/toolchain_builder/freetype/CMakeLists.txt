SET(LIBRARY_NAME freetype)
FILE(GLOB FREETYPE_HEADERS "include/ft2build.h" "include/freetype/*.h")
FILE(GLOB FREETYPE_CONFIG_HEADERS "include/freetype/config/*.h")
FILE(GLOB FREETYPE_PRIVATE_HEADERS "include/freetype/internal/*.h")

SET(FREETYPE_SOURCES
    src/autofit/autofit.c src/base/ftbase.c src/base/ftbbox.c
    src/base/ftbdf.c src/base/ftbitmap.c src/base/ftcid.c
    src/base/ftfstype.c src/base/ftgasp.c src/base/ftglyph.c
    src/base/ftgxval.c src/base/ftinit.c src/base/ftmm.c
    src/base/ftotval.c src/base/ftpatent.c src/base/ftpfr.c
    src/base/ftstroke.c src/base/ftsynth.c src/base/fttype1.c
    src/base/ftwinfnt.c src/bdf/bdf.c src/bzip2/ftbzip2.c
    src/cache/ftcache.c src/cff/cff.c src/cid/type1cid.c
    src/gzip/ftgzip.c src/lzw/ftlzw.c src/pcf/pcf.c
    src/pfr/pfr.c src/psaux/psaux.c src/pshinter/pshinter.c
    src/psnames/psnames.c src/raster/raster.c src/sfnt/sfnt.c
    src/smooth/smooth.c src/truetype/truetype.c src/type1/type1.c
    src/type42/type42.c src/winfonts/winfnt.c
)

INCLUDE_DIRECTORIES(../zlib ../png include)
FILE(READ "include/freetype/config/ftoption.h" FTOPTION_H)
STRING(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_SYSTEM_ZLIB) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")
STRING(REGEX REPLACE "/\\* +(#define +FT_CONFIG_OPTION_USE_PNG) +\\*/" "\\1" FTOPTION_H "${FTOPTION_H}")

SET(FTOPTION_H_NAME "include/freetype/config/ftoption.h")
IF(EXISTS "${FTOPTION_H_NAME}")
    FILE(READ "${FTOPTION_H_NAME}" ORIGINAL_FTOPTION_H)
ELSE()
    SET(ORIGINAL_FTOPTION_H "")
ENDIF()
IF(NOT (ORIGINAL_FTOPTION_H STREQUAL FTOPTION_H))
    FILE(WRITE "${FTOPTION_H_NAME}" "${FTOPTION_H}")
ENDIF()

IF(UNIX)
    LIST(APPEND FREETYPE_SOURCES "src/base/ftsystem.c")
ELSE()
    LIST(APPEND FREETYPE_SOURCES "src/base/ftsystem.c")
ENDIF()

IF(WIN32)
    ENABLE_LANGUAGE(RC)
    LIST(APPEND FREETYPE_SOURCES src/base/ftdebug.c src/base/ftver.rc)
ELSE()
    LIST(APPEND FREETYPE_SOURCES src/base/ftdebug.c)
ENDIF()

ADD_DEFINITIONS(-DFT2_BUILD_LIBRARY)
IF(WIN32)
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_WARNINGS)
ENDIF(WIN32)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${FREETYPE_SOURCES} ${FREETYPE_HEADERS})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")
#INSTALL(FILES ${${LIBRARY_NAME}_ZLIB_HEADERS} DESTINATION include)
