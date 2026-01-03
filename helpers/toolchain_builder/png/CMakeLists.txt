SET(LIBRARY_NAME png)
SET(PNG_HEADERS png.h pngconf.h)

SET(PNG_SOURCES
    png.c pngset.c pngget.c pngrutil.c pngtrans.c pngwutil.c
    pngread.c pngrio.c pngwio.c pngwrite.c pngrtran.c
    pngwtran.c pngmem.c pngerror.c pngpread.c)

INCLUDE_DIRECTORIES(BEFORE ../zlib .)
ADD_DEFINITIONS(-DPNG_NO_MMX_CODE -DPNG_ARM_NEON_OPT=0)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${PNG_SOURCES} ${PNG_HEADERS})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")
