SET(LIB_NAME osgdb_codec_nv)
SET(LIBRARY_FILES
    ReaderWriterCodecNV.cpp
    Utils/NvCodecUtils.h Common/FramePresenter.h
)

#########################################
FIND_DEPENDENCE(NV_DECODER "nvcuvid.h" "libnvidia-decode.so;libnvcuvid.so;nvcuvid.lib" "")
FIND_DEPENDENCE(NV_ENCODER "nvEncodeAPI.h" "libnvidia-encode.so;libnvencodeapi.so;nvencodeapi.lib" "")
IF(CUDA_FOUND)
    IF(NV_DECODER_FOUND OR NV_ENCODER_FOUND)

        SET_PROPERTY(GLOBAL APPEND PROPERTY VERSE_PLUGIN_LIBRARIES "${LIB_NAME}")
        IF(NV_DECODER_FOUND)
            ADD_DEFINITIONS(-DNV_DECODER)
            SET(LIBRARY_FILES ${LIBRARY_FILES} Common/AppDecUtils.h
                NvDecoder/NvDecoder.cpp NvDecoder/NvDecoder.h)
        ENDIF()
        IF(NV_ENCODER_FOUND)
            ADD_DEFINITIONS(-DNV_ENCODER)
            SET(LIBRARY_FILES ${LIBRARY_FILES}
                NvEncoder/NvEncoder.cpp NvEncoder/NvEncoder.h
                NvEncoder/NvEncoderCuda.cpp NvEncoder/NvEncoderCuda.h)
        ENDIF()

        IF(VERSE_STATIC_BUILD)
            NEW_CUDA_PLUGIN(${LIB_NAME} STATIC)
        ELSE()
            NEW_CUDA_PLUGIN(${LIB_NAME} SHARED)
        ENDIF()

        SET_PROPERTY(TARGET ${LIB_NAME} PROPERTY FOLDER "PLUGINS")
        TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
        TARGET_LINK_LIBRARIES(${LIB_NAME} osgVerseDependency osgVersePipeline ${CUDA_CUDA_LIBRARY})
        LINK_OSG_LIBRARY(${LIB_NAME} OpenThreads osg osgDB osgUtil)
        IF(NV_DECODER_FOUND)
            TARGET_LINK_LIBRARIES(${LIB_NAME} nvcuvid)
        ENDIF()
        IF(NV_ENCODER_FOUND)
            TARGET_LINK_LIBRARIES(${LIB_NAME} nvencodeapi)
        ENDIF()

        INSTALL(TARGETS ${LIB_NAME} EXPORT ${LIB_NAME}
                RUNTIME DESTINATION ${INSTALL_PLUGINDIR} COMPONENT libosgverse
                LIBRARY DESTINATION ${INSTALL_LIBDIR} COMPONENT libosgverse
                ARCHIVE DESTINATION ${INSTALL_ARCHIVEDIR} COMPONENT libosgverse-dev)
        IF(NOT VERSE_STATIC_BUILD)
            IF(MSVC AND VERSE_INSTALL_PDB_FILES)
                INSTALL(FILES $<TARGET_PDB_FILE:${LIB_NAME}> DESTINATION ${INSTALL_PLUGINDIR} OPTIONAL)
            ENDIF()
        ENDIF()

    ENDIF(NV_DECODER_FOUND OR NV_ENCODER_FOUND)
ENDIF(CUDA_FOUND)
