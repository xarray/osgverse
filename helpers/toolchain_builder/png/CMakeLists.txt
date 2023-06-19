SET(LIBRARY_NAME png)
SET(PNG_HEADERS png.h pngconf.h)

SET(PNG_SOURCES
    png.c pngset.c pngget.c pngrutil.c pngtrans.c pngwutil.c
    pngread.c pngrio.c pngwio.c pngwrite.c pngrtran.c
    pngwtran.c pngmem.c pngerror.c pngpread.c)

INCLUDE_DIRECTORIES(../zlib)
ADD_DEFINITIONS(-DPNG_NO_MMX_CODE)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${PNG_SOURCES} ${PNG_HEADERS})
