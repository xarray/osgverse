SET(TIFF_INT8_T "int8_t")
SET(TIFF_UINT8_T "uint8_t")
SET(TIFF_INT16_T "int16_t")
SET(TIFF_UINT16_T "uint16_t")
SET(TIFF_INT32_T "int32_t")
SET(TIFF_UINT32_T "uint32_t")
SET(TIFF_INT64_T "int64_t")
SET(TIFF_UINT64_T "uint64_t")
set(CCITT_SUPPORT ON)
set(PACKBITS_SUPPORT ON)
set(LZW_SUPPORT ON)
set(THUNDER_SUPPORT ON)
set(NEXT_SUPPORT ON)
set(LOGLUV_SUPPORT ON)
set(MDI_SUPPORT ON)
SET(STRIP_SIZE_DEFAULT 8192)

INCLUDE(CheckTypeSize)
INCLUDE(CheckIncludeFile)
INCLUDE(TestBigEndian)
CHECK_INCLUDE_FILE(assert.h    HAVE_ASSERT_H)
CHECK_INCLUDE_FILE(fcntl.h     HAVE_FCNTL_H)
CHECK_INCLUDE_FILE(io.h        HAVE_IO_H)
CHECK_INCLUDE_FILE(strings.h   HAVE_STRINGS_H)
CHECK_INCLUDE_FILE(sys/time.h  HAVE_SYS_TIME_H)
CHECK_INCLUDE_FILE(sys/types.h HAVE_SYS_TYPES_H)
CHECK_INCLUDE_FILE(unistd.h    HAVE_UNISTD_H)
CHECK_TYPE_SIZE("size_t" SIZEOF_SIZE_T)
TEST_BIG_ENDIAN(HOST_BIG_ENDIAN)
IF(HOST_BIG_ENDIAN)
    ADD_DEFINITIONS(-DWORDS_BIGENDIAN)
ENDIF()

if(SIZEOF_SIZE_T EQUAL 4)
    set(TIFF_SSIZE_T "int32_t")
elseif(SIZEOF_SIZE_T EQUAL 8)
    set(TIFF_SSIZE_T "int64_t")
else()
    message(FATAL_ERROR "Unsupported size_t size ${SIZEOF_SIZE_T}; please add support")
endif()

SET(HOST_FILLORDER FILLORDER_MSB2LSB)
IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i.*86.*" OR
   CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "amd64.*" OR
   CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64" OR
   CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64.*")
    SET(HOST_FILLORDER FILLORDER_LSB2MSB)
ENDIF()
SET(HAVE_IEEEFP 1)
SET(JPEG_SUPPORT 1)
SET(ZIP_SUPPORT 1)
IF(WIN32)
    ADD_DEFINITIONS(-DUSE_WIN32_FILEIO)
    SET(USE_WIN32_FILEIO 1)
ENDIF()
CONFIGURE_FILE(tif_config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tif_config.h @ONLY)
CONFIGURE_FILE(tiffconf.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/tiffconf.h @ONLY)

SET(LIBRARY_NAME tiff)
SET(TIFF_HEADERS
    tiff.h tiffio.h tiffvers.h
    ${CMAKE_CURRENT_BINARY_DIR}/tiffconf.h)

SET(TIFF_PRIVATE_HEADERS
    t4.h tif_dir.h tif_predict.h tiffiop.h uvcode.h
    ${CMAKE_CURRENT_BINARY_DIR}/tif_config.h)

SET(TIFF_SOURCES
    tif_aux.c tif_close.c tif_codec.c tif_color.c
    tif_compress.c tif_dir.c tif_dirinfo.c tif_dirread.c
    tif_dirwrite.c tif_dumpmode.c tif_error.c
    tif_extension.c tif_fax3.c tif_fax3sm.c tif_flush.c
    tif_getimage.c tif_jbig.c tif_jpeg.c tif_jpeg_12.c
    tif_lerc.c tif_luv.c tif_lzma.c tif_lzw.c tif_next.c
    tif_ojpeg.c tif_open.c tif_packbits.c tif_pixarlog.c
    tif_predict.c tif_print.c tif_read.c tif_strip.c
    tif_swab.c tif_thunder.c tif_tile.c tif_version.c
    tif_warning.c tif_webp.c tif_write.c tif_zip.c tif_zstd.c)

IF(WIN32)
    LIST(APPEND TIFF_SOURCES tif_win32.c)
ELSE()
    LIST(APPEND TIFF_SOURCES tif_unix.c)
ENDIF()

INCLUDE_DIRECTORIES(../zlib ../jpeg ${CMAKE_CURRENT_BINARY_DIR})
ADD_DEFINITIONS(-DTIFF_DISABLE_DEPRECATED)
ADD_LIBRARY(${LIBRARY_NAME} STATIC ${TIFF_SOURCES} ${TIFF_HEADERS})
SET_PROPERTY(TARGET ${LIBRARY_NAME} PROPERTY FOLDER "THIRD_PARTY")
#INSTALL(FILES ${${LIBRARY_NAME}_ZLIB_HEADERS} DESTINATION include)
