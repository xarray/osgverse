SET(LIBRARY_NAME zlib)
SET(ZLIB_HEADERS zconf.h zlib.h)

SET(ZLIB_SOURCES
    adler32.c compress.c crc32.c
    deflate.c gzclose.c gzlib.c
    gzread.c gzwrite.c infback.c
    inffast.c inflate.c inftrees.c
    trees.c uncompr.c zutil.c)

INCLUDE_DIRECTORIES(BEFORE .)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${ZLIB_SOURCES} ${ZLIB_HEADERS})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")
#INSTALL(FILES ${${LIBRARY_NAME}_ZLIB_HEADERS} DESTINATION include)
