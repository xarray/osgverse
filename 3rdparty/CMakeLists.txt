CONFIGURE_FILE(leveldb/port/port_config.h.in "${CMAKE_BINARY_DIR}/3rdparty/leveldb/port/port_config.h")
SET(LEVELDB_SOURCE_FILES
    leveldb/db/builder.cc leveldb/db/builder.h leveldb/db/c.cc
    leveldb/db/db_impl.cc leveldb/db/db_impl.h leveldb/db/db_iter.cc leveldb/db/db_iter.h
    leveldb/db/dbformat.cc leveldb/db/dbformat.h leveldb/db/dumpfile.cc
    leveldb/db/filename.cc leveldb/db/filename.h leveldb/db/log_format.h
    leveldb/db/log_reader.cc leveldb/db/log_reader.h leveldb/db/log_writer.cc leveldb/db/log_writer.h
    leveldb/db/memtable.cc leveldb/db/memtable.h leveldb/db/repair.cc
    leveldb/db/skiplist.h leveldb/db/snapshot.h leveldb/db/table_cache.cc leveldb/db/table_cache.h
    leveldb/db/version_edit.cc leveldb/db/version_edit.h leveldb/db/version_set.cc leveldb/db/version_set.h
    leveldb/db/write_batch_internal.h leveldb/db/write_batch.cc
    leveldb/port/port_stdcxx.h leveldb/port/port.h leveldb/port/thread_annotations.h
    leveldb/table/block_builder.cc leveldb/table/block_builder.h
    leveldb/table/block.cc leveldb/table/block.h leveldb/table/filter_block.cc leveldb/table/filter_block.h
    leveldb/table/format.cc leveldb/table/format.h leveldb/table/iterator_wrapper.h leveldb/table/iterator.cc
    leveldb/table/merger.cc leveldb/table/merger.h leveldb/table/table_builder.cc leveldb/table/table.cc
    leveldb/table/two_level_iterator.cc leveldb/table/two_level_iterator.h leveldb/util/coding.cc
    leveldb/util/arena.cc leveldb/util/arena.h leveldb/util/bloom.cc leveldb/util/cache.cc
    leveldb/util/coding.h leveldb/util/comparator.cc leveldb/util/crc32c.cc leveldb/util/crc32c.h
    leveldb/util/env.cc leveldb/util/filter_policy.cc leveldb/util/hash.cc leveldb/util/hash.h
    leveldb/util/logging.cc leveldb/util/logging.h leveldb/util/mutexlock.h leveldb/util/no_destructor.h
    leveldb/util/options.cc leveldb/util/random.h leveldb/util/status.cc
    leveldb/helpers/memenv/memenv.cc leveldb/helpers/memenv/memenv.h
)
IF(WIN32)
    SET(LEVELDB_SOURCE_FILES ${LEVELDB_SOURCE_FILES} leveldb/util/env_windows.cc leveldb/util/windows_logger.h)
ELSE(WIN32)
    SET(LEVELDB_SOURCE_FILES ${LEVELDB_SOURCE_FILES} leveldb/util/env_posix.cc leveldb/util/posix_logger.h)
ENDIF(WIN32)

SET(ApproxMVBB_VERSION_MAJOR 3)
SET(ApproxMVBB_VERSION_MINOR 1)
SET(ApproxMVBB_VERSION_PATCH 2)
SET(ApproxMVBB_FORCE_MSGLOG_LEVEL 0)
CONFIGURE_FILE(ApproxMVBB/GeometryPredicates/Config.hpp.in.cmake
               "${CMAKE_BINARY_DIR}/3rdparty/ApproxMVBB/GeometryPredicates/Config.hpp")
CONFIGURE_FILE(ApproxMVBB/Config/Config.hpp.in.cmake
               "${CMAKE_BINARY_DIR}/3rdparty/ApproxMVBB/Config/Config.hpp")
SET(APPROXMVBB_SOURCE_FILES
    ApproxMVBB/AABB.cpp ApproxMVBB/AABB.hpp ApproxMVBB/AngleFunctions.hpp
    ApproxMVBB/ComputeApproxMVBB.hpp ApproxMVBB/ContainerFunctions.hpp
    ApproxMVBB/ConvexHull2D.cpp ApproxMVBB/ConvexHull2D.hpp
    ApproxMVBB/GreatestCommonDivisor.hpp ApproxMVBB/KdTree.hpp
    ApproxMVBB/KdTreeXml.hpp ApproxMVBB/MakeCoordinateSystem.hpp
    ApproxMVBB/MinAreaRectangle.cpp ApproxMVBB/MinAreaRectangle.hpp
    ApproxMVBB/OOBB.cpp ApproxMVBB/OOBB.hpp ApproxMVBB/PointFunctions.hpp
    ApproxMVBB/ProjectedPointSet.cpp ApproxMVBB/ProjectedPointSet.hpp
    ApproxMVBB/RandomGenerators.cpp ApproxMVBB/RandomGenerators.hpp
    ApproxMVBB/Common/AssertionDebug.hpp ApproxMVBB/Common/ContainerTag.hpp
    ApproxMVBB/Common/CPUTimer.hpp ApproxMVBB/Common/CygwinPatch.hpp
    ApproxMVBB/Common/Exception.hpp ApproxMVBB/Common/FloatingPointComparision.hpp
    ApproxMVBB/Common/LogDefines.hpp ApproxMVBB/Common/MyContainerTypeDefs.hpp
    ApproxMVBB/Common/MyMatrixTypeDefs.cpp ApproxMVBB/Common/MyMatrixTypeDefs.hpp
    ApproxMVBB/Common/Platform.hpp ApproxMVBB/Common/SfinaeMacros.hpp
    ApproxMVBB/Common/StaticAssert.hpp ApproxMVBB/Common/TypeDefs.hpp
    ApproxMVBB/Common/TypeDefsPoints.hpp ApproxMVBB/Diameter/alloc.cpp
    ApproxMVBB/Diameter/EstimateDiameter.cpp ApproxMVBB/Diameter/rand.cpp ApproxMVBB/Diameter/util.cpp
    ApproxMVBB/GeometryPredicates/Predicates.cpp ApproxMVBB/GeometryPredicates/PredicatesInit.cpp
)

SET(NORMALMAP_SOURCE_FILES
    normalmap/boxblur.cpp normalmap/boxblur.h
    normalmap/gaussianblur.cpp normalmap/gaussianblur.h
    normalmap/intensitymap.cpp normalmap/intensitymap.h
    normalmap/normalmapgenerator.cpp normalmap/normalmapgenerator.h
    normalmap/specularmapgenerator.cpp normalmap/specularmapgenerator.h
    normalmap/ssaogenerator.cpp normalmap/ssaogenerator.h
)

SET(LASZIP_SOURCE_FILES
    laszip/arithmeticdecoder.cpp laszip/arithmeticencoder.cpp
    laszip/arithmeticmodel.cpp laszip/integercompressor.cpp
    laszip/lasindex.cpp laszip/lasinterval.cpp laszip/lasquadtree.cpp
    laszip/lasreaditemcompressed_v1.cpp laszip/lasreaditemcompressed_v2.cpp
    laszip/lasreaditemcompressed_v3.cpp laszip/lasreaditemcompressed_v4.cpp
    laszip/laswriteitemcompressed_v1.cpp laszip/laswriteitemcompressed_v2.cpp
    laszip/laswriteitemcompressed_v3.cpp laszip/laswriteitemcompressed_v4.cpp
    laszip/lasreadpoint.cpp laszip/laswritepoint.cpp laszip/mydefs.cpp
    laszip/laszip.cpp laszip/laszip_dll.cpp
)

SET(OZZ_SOURCE_FILES
    ozz/base/memory/allocator.cc ozz/base/platform.cc ozz/base/log.cc
    ozz/base/containers/string_archive.cc ozz/base/io/archive.cc ozz/base/io/stream.cc
    ozz/base/maths/box.cc ozz/base/maths/simd_math.cc ozz/base/maths/math_archive.cc
    ozz/base/maths/soa_math_archive.cc ozz/base/maths/simd_math_archive.cc
    ozz/animation/runtime/animation.cc ozz/animation/runtime/animation_keyframe.h
    ozz/animation/runtime/animation_utils.cc ozz/animation/runtime/blending_job.cc
    ozz/animation/runtime/ik_aim_job.cc ozz/animation/runtime/ik_two_bone_job.cc
    ozz/animation/runtime/local_to_model_job.cc ozz/animation/runtime/sampling_job.cc
    ozz/animation/runtime/skeleton.cc ozz/animation/runtime/skeleton_utils.cc
    ozz/animation/runtime/track.cc ozz/animation/runtime/track_sampling_job.cc
    ozz/animation/runtime/track_triggering_job.cc ozz/geometry/runtime/skinning_job.cc
    ozz/options/options.cc ozz/mesh.cc
)

SET(PMP_MCUT_SOURCE_FILES
    pmp/BoundingBox.h pmp/Exceptions.h pmp/MatVec.h
    pmp/MemoryUsage.h pmp/Properties.h pmp/Timer.h pmp/Types.h
    pmp/SurfaceMesh.cpp pmp/SurfaceMesh.h pmp/utilities.cpp pmp/utilities.h
    pmp/algorithms/curvature.cpp pmp/algorithms/curvature.h
    pmp/algorithms/decimation.cpp pmp/algorithms/decimation.h
    #pmp/algorithms/fairing.cpp pmp/algorithms/fairing.h
    pmp/algorithms/features.cpp pmp/algorithms/features.h
    pmp/algorithms/geodesics.cpp pmp/algorithms/geodesics.h
    pmp/algorithms/hole_filling.cpp pmp/algorithms/hole_filling.h
    pmp/algorithms/normals.cpp pmp/algorithms/normals.h
    pmp/algorithms/parameterization.cpp pmp/algorithms/parameterization.h
    pmp/algorithms/remeshing.cpp pmp/algorithms/remeshing.h
    pmp/algorithms/shapes.cpp pmp/algorithms/shapes.h
    pmp/algorithms/smoothing.cpp pmp/algorithms/smoothing.h
    pmp/algorithms/subdivision.cpp pmp/algorithms/subdivision.h
    pmp/algorithms/triangulation.cpp pmp/algorithms/triangulation.h
    pmp/algorithms/DifferentialGeometry.cpp pmp/algorithms/DifferentialGeometry.h
    pmp/algorithms/DistancePointTriangle.cpp pmp/algorithms/DistancePointTriangle.h
    pmp/algorithms/BarycentricCoordinates.h pmp/algorithms/DistancePointTriangle.h
    mcut/bvh.cpp mcut/frontend.cpp mcut/hmesh.cpp
    mcut/kernel.cpp mcut/math.cpp mcut/mcut.cpp mcut/mcut.h
    mcut/preproc.cpp mcut/platform.h mcut/shewchuk.c
)

SET(IMGUI_SOURCE_FILES
    imgui/imgui-knobs.cpp imgui/imgui-knobs.h
    imgui/implot_items.cpp imgui/implot.cpp imgui/implot.h
    imgui/GraphEditor.cpp imgui/GraphEditor.h
    imgui/ImCurveEdit.cpp imgui/ImCurveEdit.h
    imgui/ImGradient.cpp imgui/ImGradient.h
    imgui/ImSequencer.cpp imgui/ImSequencer.h
    imgui/ImGuizmo.cpp imgui/ImGuizmo.h imgui/ImZoomSlider.h
    imgui/ImGuiFileDialog.cpp imgui/ImGuiFileDialog.h
    imgui/ImGuiFileDialogConfig.h
	
	imgui/node-editor/imgui_bezier_math.h imgui/node-editor/imgui_extra_math.h
	imgui/node-editor/crude_json.cpp imgui/node-editor/crude_json.h
	imgui/node-editor/imgui_canvas.cpp imgui/node-editor/imgui_canvas.h
	imgui/node-editor/imgui_node_editor.cpp imgui/node-editor/imgui_node_editor.h
	imgui/node-editor/imgui_node_editor_api.cpp
    
    imgui/imgui.cpp imgui/imgui.h imgui/imconfig.h
    imgui/imgui_demo.cpp imgui/imgui_draw.cpp
    imgui/imgui_tables.cpp imgui/imgui_widgets.cpp
	imgui/imgui_impl_opengl3.cpp imgui/imgui_impl_opengl3.h
)
IF(VERSE_USE_EXTERNAL_GLES2 OR VERSE_USE_EXTERNAL_GLES3)
    #SET ?
ELSE()
    SET(IMGUI_SOURCE_FILES ${IMGUI_SOURCE_FILES}
		imgui/imgui_impl_opengl2.cpp imgui/imgui_impl_opengl2.h)
ENDIF()

SET(PINYIN_SOURCE_FILES
    pinyin/Cache.cpp pinyin/CandidateStubs.cpp
    pinyin/Ciku.cpp pinyin/CikuManager.cpp
    pinyin/Pinyin.cpp pinyin/PinyinBase.cpp
    pinyin/PinyinDivider.cpp pinyin/PinyinHanzi.cpp
    pinyin/Pinyin_Internal.cpp pinyin/Query.cpp
    pinyin/StringFunction.cpp pinyin/Pinyin.h
)

SET(TINYSPLINE_SOURCE_FILES
    tinyspline/parson.c tinyspline/parson.h
    tinyspline/tinyspline.c tinyspline/tinyspline.h
    tinyspline/tinysplinecxx.cxx tinyspline/tinysplinecxx.h
)

SET(MBEDTLS_SOURCE_FILES
    libhv/everest/everest.c libhv/everest/x25519.c libhv/everest/Hacl_Curve25519_joined.c
    libhv/mbedtls/aes.c libhv/mbedtls/aesni.c libhv/mbedtls/aria.c
    libhv/mbedtls/asn1parse.c libhv/mbedtls/asn1write.c libhv/mbedtls/base64.c
    libhv/mbedtls/bignum.c libhv/mbedtls/bignum_core.c libhv/mbedtls/bignum_mod.c
    libhv/mbedtls/bignum_mod_raw.c libhv/mbedtls/camellia.c libhv/mbedtls/ccm.c
    libhv/mbedtls/chacha20.c libhv/mbedtls/chachapoly.c libhv/mbedtls/cipher.c
    libhv/mbedtls/cipher_wrap.c libhv/mbedtls/constant_time.c libhv/mbedtls/cmac.c
    libhv/mbedtls/ctr_drbg.c libhv/mbedtls/des.c libhv/mbedtls/dhm.c libhv/mbedtls/ecdh.c
    libhv/mbedtls/ecdsa.c libhv/mbedtls/ecjpake.c libhv/mbedtls/ecp.c libhv/mbedtls/ecp_curves.c
    libhv/mbedtls/entropy.c libhv/mbedtls/entropy_poll.c libhv/mbedtls/error.c libhv/mbedtls/gcm.c
    libhv/mbedtls/hash_info.c libhv/mbedtls/hkdf.c libhv/mbedtls/hmac_drbg.c libhv/mbedtls/lmots.c
    libhv/mbedtls/lms.c libhv/mbedtls/md.c libhv/mbedtls/md5.c libhv/mbedtls/memory_buffer_alloc.c
    libhv/mbedtls/nist_kw.c libhv/mbedtls/oid.c libhv/mbedtls/padlock.c libhv/mbedtls/pem.c
    libhv/mbedtls/pk.c libhv/mbedtls/pk_wrap.c libhv/mbedtls/pkcs12.c libhv/mbedtls/pkcs5.c
    libhv/mbedtls/pkparse.c libhv/mbedtls/pkwrite.c libhv/mbedtls/platform.c libhv/mbedtls/platform_util.c
    libhv/mbedtls/poly1305.c libhv/mbedtls/psa_crypto.c libhv/mbedtls/psa_crypto_aead.c
    libhv/mbedtls/psa_crypto_cipher.c libhv/mbedtls/psa_crypto_client.c
    libhv/mbedtls/psa_crypto_driver_wrappers.c libhv/mbedtls/psa_crypto_ecp.c
    libhv/mbedtls/psa_crypto_hash.c libhv/mbedtls/psa_crypto_mac.c libhv/mbedtls/psa_crypto_pake.c
    libhv/mbedtls/psa_crypto_rsa.c libhv/mbedtls/psa_crypto_se.c libhv/mbedtls/psa_crypto_slot_management.c
    libhv/mbedtls/psa_crypto_storage.c libhv/mbedtls/psa_its_file.c libhv/mbedtls/ripemd160.c
    libhv/mbedtls/rsa.c libhv/mbedtls/rsa_alt_helpers.c libhv/mbedtls/sha1.c libhv/mbedtls/sha256.c
    libhv/mbedtls/sha512.c libhv/mbedtls/threading.c libhv/mbedtls/timing.c
    libhv/mbedtls/version.c libhv/mbedtls/version_features.c
    libhv/mbedtls/pkcs7.c libhv/mbedtls/x509.c libhv/mbedtls/x509_create.c libhv/mbedtls/x509_crl.c
    libhv/mbedtls/x509_crt.c libhv/mbedtls/x509_csr.c libhv/mbedtls/x509write_crt.c
    libhv/mbedtls/x509write_csr.c
    libhv/mbedtls/debug.c libhv/mbedtls/mps_reader.c libhv/mbedtls/mps_trace.c libhv/mbedtls/net_sockets.c
    libhv/mbedtls/ssl_cache.c libhv/mbedtls/ssl_ciphersuites.c libhv/mbedtls/ssl_client.c
    libhv/mbedtls/ssl_cookie.c libhv/mbedtls/ssl_debug_helpers_generated.c libhv/mbedtls/ssl_msg.c
    libhv/mbedtls/ssl_ticket.c libhv/mbedtls/ssl_tls.c libhv/mbedtls/ssl_tls12_client.c
    libhv/mbedtls/ssl_tls12_server.c libhv/mbedtls/ssl_tls13_keys.c libhv/mbedtls/ssl_tls13_server.c
    libhv/mbedtls/ssl_tls13_client.c libhv/mbedtls/ssl_tls13_generic.c
)

SET(LIBHV_SOURCE_FILES
    libhv/all/hbase.c libhv/all/herr.c libhv/all/hlog.c libhv/all/hmain.c
    libhv/all/hsocket.c libhv/all/htime.c libhv/all/hversion.c libhv/all/rbtree.c
    libhv/all/epoll.c libhv/all/evport.c libhv/all/hevent.c libhv/all/hloop.c
    libhv/all/iocp.c libhv/all/kqueue.c libhv/all/nio.c libhv/all/nlog.c
    libhv/all/noevent.c libhv/all/overlapio.c libhv/all/poll.c libhv/all/rudp.c
    libhv/all/select.c libhv/all/unpack.c libhv/all/kcp/hkcp.c libhv/all/kcp/ikcp.c
    libhv/all/httpdef.c libhv/all/http_parser.c libhv/all/multipart_parser.c
    libhv/all/websocket_parser.c libhv/all/wsdef.c libhv/all/mqtt_client.c libhv/all/mqtt_protocol.c
    libhv/all/dns.c libhv/all/ftp.c libhv/all/icmp.c libhv/all/smtp.c
    libhv/all/appletls.c libhv/all/gnutls.c libhv/all/hssl.c libhv/all/mbedtls.c
    libhv/all/nossl.c libhv/all/openssl.c libhv/all/wintls.c libhv/all/base64.c
    libhv/all/md5.c libhv/all/sha1.c
    libhv/all/hasync.cpp libhv/all/hdir.cpp libhv/all/hpath.cpp libhv/all/hstring.cpp
    libhv/all/hurl.cpp libhv/all/ifconfig.cpp libhv/all/iniparser.cpp libhv/all/RAII.cpp
    libhv/all/ThreadLocalStorage.cpp
    #libhv/all/EventLoopThreadPool_test.cpp libhv/all/EventLoopThread_test.cpp
    #libhv/all/EventLoop_test.cpp libhv/all/TcpClientEventLoop_test.cpp
    #libhv/all/TcpClient_test.cpp libhv/all/TcpServer_test.cpp
    #libhv/all/TimerThread_test.cpp libhv/all/UdpClient_test.cpp libhv/all/UdpServer_test.cpp
    libhv/all/Http1Parser.cpp libhv/all/Http2Parser.cpp libhv/all/HttpMessage.cpp
    libhv/all/HttpParser.cpp libhv/all/http_content.cpp
    libhv/all/WebSocketChannel.cpp libhv/all/WebSocketParser.cpp
    libhv/all/client/AsyncHttpClient.cpp libhv/all/client/WebSocketClient.cpp
    libhv/all/server/FileCache.cpp libhv/all/server/HttpHandler.cpp
    libhv/all/server/HttpServer.cpp libhv/all/server/HttpService.cpp
    libhv/all/client/http_client.cpp libhv/all/server/http_page.cpp libhv/hv.h
)

SOURCE_GROUP("LevelDB Files" FILES ${LEVELDB_SOURCE_FILES})
SOURCE_GROUP("ApproxMVBB Files" FILES ${APPROXMVBB_SOURCE_FILES})
SOURCE_GROUP("NormalMap Files" FILES ${NORMALMAP_SOURCE_FILES})
SOURCE_GROUP("VHACD Files" FILES ${VHACD_SOURCE_FILES})
SOURCE_GROUP("LasZIP Files" FILES ${LASZIP_SOURCE_FILES})
SOURCE_GROUP("OzzAnimation Files" FILES ${OZZ_SOURCE_FILES})
SOURCE_GROUP("PMP & MCut Files" FILES ${PMP_MCUT_SOURCE_FILES})
SOURCE_GROUP("ImGUI Files" FILES ${IMGUI_SOURCE_FILES})
SOURCE_GROUP("PinYin-IME Files" FILES ${PINYIN_SOURCE_FILES})
SOURCE_GROUP("TinySpline Files" FILES ${TINYSPLINE_SOURCE_FILES})
SOURCE_GROUP("MbedTLS Files" FILES ${MBEDTLS_SOURCE_FILES})
SOURCE_GROUP("LibHV Files" FILES ${LIBHV_SOURCE_FILES})
#######################

SET(LIB_NAME osgVerseDependency)
SET(LIBRARY_FILES
	tiny_obj_loader.h tiny_gltf.h picojson.h nanoflann.hpp exprtk.hpp mio.hpp any.hpp
    backward.hpp strtk.hpp tinydir.h rapidxml/rapidxml.hpp rapidjson/rapidjson.h
    nanoid/nanoid.cpp nanoid/nanoid.h nanoid/crypto_random.cpp nanoid/crypto_random.h
    sqlite3.c sqlite3.h tinyexr.cc tinyexr.h miniz.c miniz.h ofbx.cpp ofbx.h
    mikktspace.c mikktspace.h laplacian_deformation.cpp laplacian_deformation.hpp
	mimalloc/static.c mimalloc/mimalloc.h mimalloc/mimalloc-new-delete.h
	microprofile.cpp microprofile.h xatlas.cpp xatlas.h easing.cpp easing.h
    lightmapper.h GL/glew.c GL/glew.h xxYUV/rgb2yuv.cpp xxYUV/yuv2rgb.cpp xxYUV/cpu.cpp
    ${LEVELDB_SOURCE_FILES} ${APPROXMVBB_SOURCE_FILES} ${NORMALMAP_SOURCE_FILES}
    ${PMP_MCUT_SOURCE_FILES} ${OZZ_SOURCE_FILES} ${LASZIP_SOURCE_FILES}
    ${IMGUI_SOURCE_FILES} ${PINYIN_SOURCE_FILES} ${TINYSPLINE_SOURCE_FILES}
    ${MBEDTLS_SOURCE_FILES} ${LIBHV_SOURCE_FILES}
)

ADD_DEFINITIONS(-DApproxMVBB_BUILD_LIBRARY -DGLEW_STATIC -DGLEW_NO_GLU -DLASZIPDLL_EXPORTS
                -DLEVELDB_COMPILE_LIBRARY -DLEVELDB_HAS_PORT_CONFIG_H=1 -DHV_STATICLIB -DWITH_MBEDTLS 
				-DIMGUI_IMPL_OPENGL_LOADER_GLEW -DUNORDERED -DHAVE_UNORDERED_MAP=1)
IF (MSVC_VERSION LESS 1910)
    ADD_DEFINITIONS(-DWITHOUT_HTTP_CONTENT)  # disable use of json.hpp in VS2015 or lower
ENDIF()

IF(WIN32)
    ADD_DEFINITIONS(-DLEVELDB_PLATFORM_WINDOWS)
ELSE()
    ADD_DEFINITIONS(-DLEVELDB_PLATFORM_POSIX)
ENDIF()
IF(VERSE_USE_EXTERNAL_GLES2)
    ADD_DEFINITIONS(-DGLEW_EGL -DIMGUI_IMPL_OPENGL_ES2)
ELSEIF(VERSE_USE_EXTERNAL_GLES3)
    ADD_DEFINITIONS(-DGLEW_EGL -DIMGUI_IMPL_OPENGL_ES3)
ENDIF()

INCLUDE_DIRECTORIES(libhv libhv/all libhv/mbedtls imgui stb ozz leveldb
                    "${CMAKE_BINARY_DIR}/3rdparty/leveldb")
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

SET(THIRDPARTY_INCLUDE_FILES any.hpp strtk.hpp backward.hpp)
SET(IMGUI_INCLUDE_FILES imgui/imgui.h imgui/imconfig.h imgui/ImGuizmo.h)
INSTALL(FILES ${THIRDPARTY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse COMPONENT libosgverse-dev)
INSTALL(FILES ${IMGUI_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/imgui COMPONENT libosgverse-dev)
