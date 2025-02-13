SET(LIB_NAME osgdb_codec_ffmpeg)
SET(LIBRARY_FILES
    ReaderWriterFFmpeg.cpp
    FFmpegClocks.cpp FFmpegClocks.hpp
    FFmpegDecoderAudio.cpp FFmpegDecoderAudio.hpp
    FFmpegDecoder.cpp FFmpegDecoder.hpp
    FFmpegDecoderVideo.cpp FFmpegDecoderVideo.hpp
    FFmpegImageStream.cpp FFmpegImageStream.hpp
    FFmpegAudioStream.cpp FFmpegAudioStream.hpp
    FFmpegParameters.cpp FFmpegParameters.hpp
    FFmpegHeaders.hpp FFmpegPacket.hpp
    BoundedMessageQueue.hpp MessageQueue.hpp
)

#########################################
IF(FFMPEG_FOUND)

    SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
    ADD_DEFINITIONS(-D__STDC_CONSTANT_MACROS -DUSE_SWSCALE -DUSE_SWRESAMPLE)
    IF(VERSE_STATIC_BUILD)
        NEW_PLUGIN(${LIB_NAME} STATIC)
    ELSE()
        NEW_PLUGIN(${LIB_NAME} SHARED)
    ENDIF()

    SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
    TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
    TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency avcodec avformat avdevice avutil swscale swresample)
    LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)
    IF(MSVC)
        SET_TARGET_PROPERTIES(${LIB_NAME} PROPERTIES LINK_FLAGS "/OPT:NOREF")
    ENDIF(MSVC)

    INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
            RUNTIME DESTINATION ${INSTALL_PLUGINDIR} COMPONENT libosgverse
            LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
            ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
    IF(NOT VERSE_STATIC_BUILD)
        IF(MSVC AND VERSE_INSTALL_PDB_FILES)
            INSTALL(FILES $<TARGET_PDB_FILE:${LIB_NAME}> DESTINATION ${INSTALL_PLUGINDIR} OPTIONAL)
        ENDIF()
    ENDIF()

ENDIF(FFMPEG_FOUND)
