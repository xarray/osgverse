SET(LIBRARY_NAME jpeg)
SET(JPEG_HEADERS jconfig.h jerror.h jmorecfg.h jpeglib.h)

SET(JPEG_SOURCES
    jaricom.c jcapimin.c jcapistd.c jcarith.c jccoefct.c jccolor.c
    jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c
    jcomapi.c jcparam.c jcprepct.c jcsample.c jctrans.c jdapimin.c
    jdapistd.c jdarith.c jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c
    jddctmgr.c jdhuff.c jdinput.c jdmainct.c jdmarker.c jdmaster.c
    jdmerge.c jdpostct.c jdsample.c jdtrans.c jerror.c jfdctflt.c
    jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c jquant1.c
    jquant2.c jutils.c jmemmgr.c jmemansi.c #jmemname.c jmemnobs.c
    #jmemdos.c jmemmac.c
)

INCLUDE_DIRECTORIES(BEFORE .)
ADD_DEFINITIONS(-DALL_STATIC)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${JPEG_SOURCES} ${JPEG_HEADERS})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")
