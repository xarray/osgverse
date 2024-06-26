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
    pmp/bounding_box.h pmp/exceptions.h pmp/mat_vec.h pmp/memory_usage.h
    pmp/properties.h pmp/stop_watch.h pmp/types.h pmp/surface_mesh.cpp pmp/surface_mesh.h
    pmp/algorithms/barycentric_coordinates.h
    pmp/algorithms/curvature.cpp pmp/algorithms/curvature.h
    pmp/algorithms/decimation.cpp pmp/algorithms/decimation.h
    pmp/algorithms/differential_geometry.cpp pmp/algorithms/differential_geometry.h
    pmp/algorithms/distance_point_triangle.cpp pmp/algorithms/distance_point_triangle.h
    pmp/algorithms/fairing.cpp pmp/algorithms/fairing.h
    pmp/algorithms/features.cpp pmp/algorithms/features.h
    pmp/algorithms/geodesics.cpp pmp/algorithms/geodesics.h
    pmp/algorithms/hole_filling.cpp pmp/algorithms/hole_filling.h
    pmp/algorithms/laplace.cpp pmp/algorithms/laplace.h
    pmp/algorithms/normals.cpp pmp/algorithms/normals.h
    pmp/algorithms/numerics.cpp pmp/algorithms/numerics.h
    pmp/algorithms/parameterization.cpp pmp/algorithms/parameterization.h
    pmp/algorithms/remeshing.cpp pmp/algorithms/remeshing.h
    pmp/algorithms/shapes.cpp pmp/algorithms/shapes.h
    pmp/algorithms/smoothing.cpp pmp/algorithms/smoothing.h
    pmp/algorithms/subdivision.cpp pmp/algorithms/subdivision.h
    pmp/algorithms/triangulation.cpp pmp/algorithms/triangulation.h
    pmp/algorithms/utilities.cpp pmp/algorithms/utilities.h
)

SET(IMGUI_SOURCE_FILES
    imgui/imgui-knobs.cpp imgui/imgui-knobs.h
    imgui/implot_items.cpp imgui/implot.cpp imgui/implot.h
    imgui/GraphEditor.cpp imgui/GraphEditor.h
    imgui/ImCurveEdit.cpp imgui/ImCurveEdit.h
    imgui/ImGradient.cpp imgui/ImGradient.h
    imgui/ImSequencer.cpp imgui/ImSequencer.h
    imgui/ImGuizmo.cpp imgui/ImGuizmo.h imgui/ImZoomSlider.h
    imgui/ImGuiFileDialog.cpp imgui/ImGuiFileDialog.h imgui/ImGuiFileDialogConfig.h
    imgui/imgui_dock.cpp imgui/imgui_dock.h imgui/ImFileDialog.cpp imgui/ImFileDialog.h

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
    #
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

SET(KTX_SOURCE_FILES
    ktx/basisu/transcoder/basisu_containers.h ktx/basisu/transcoder/basisu_containers_impl.h
    ktx/basisu/transcoder/basisu_file_headers.h ktx/basisu/transcoder/basisu_transcoder_internal.h
    ktx/basisu/transcoder/basisu_transcoder_uastc.h ktx/basisu/transcoder/basisu_transcoder.cpp
    ktx/basisu/transcoder/basisu_transcoder.h ktx/basisu/transcoder/basisu.h ktx/basisu/zstd/zstd.c
    ktx/dfdutils/createdfd.c ktx/dfdutils/colourspaces.c ktx/dfdutils/dfd.h ktx/dfdutils/interpretdfd.c
    ktx/dfdutils/printdfd.c ktx/dfdutils/queries.c ktx/dfdutils/vk2dfd.c ktx/dfdutils/vk2dfd.inl
    ktx/dfdutils/vulkan/vk_platform.h ktx/dfdutils/vulkan/vulkan_core.h
    ktx/ktx.h ktx/basis_sgd.h ktx/basis_transcode.cpp ktx/checkheader.c ktx/etcdec.cxx ktx/etcunpack.cxx
    ktx/filestream.c ktx/filestream.h ktx/formatsize.h ktx/gl_format.h ktx/hashlist.c ktx/info.c
    ktx/ktxint.h ktx/memstream.c ktx/memstream.h ktx/strings.c ktx/swap.c ktx/texture.c ktx/texture.h
    ktx/texture2.c ktx/texture2.h ktx/uthash.h ktx/vk_format.h ktx/vkformat_check.c
    ktx/vkformat_enum.h ktx/vkformat_str.c ktx/gl_funcs.c ktx/gl_funcs.h ktx/glloader.c
    ktx/texture1.c ktx/texture1.h ktx/basisu/encoder/basisu_backend.cpp ktx/basisu/encoder/basisu_backend.h
    ktx/basis_encode.cpp ktx/astc_encode.cpp ktx/writer1.c ktx/writer2.c
    ktx/basisu/encoder/basisu_basis_file.cpp ktx/basisu/encoder/basisu_basis_file.h
    ktx/basisu/encoder/basisu_bc7enc.cpp ktx/basisu/encoder/basisu_bc7enc.h
    ktx/basisu/encoder/basisu_comp.cpp ktx/basisu/encoder/basisu_comp.h
    ktx/basisu/encoder/basisu_enc.cpp ktx/basisu/encoder/basisu_enc.h
    ktx/basisu/encoder/basisu_etc.cpp ktx/basisu/encoder/basisu_etc.h
    ktx/basisu/encoder/basisu_frontend.cpp ktx/basisu/encoder/basisu_frontend.h
    ktx/basisu/encoder/basisu_gpu_texture.cpp ktx/basisu/encoder/basisu_gpu_texture.h
    ktx/basisu/encoder/basisu_kernels_declares.h ktx/basisu/encoder/basisu_kernels_imp.h
    ktx/basisu/encoder/basisu_kernels_sse.cpp ktx/basisu/encoder/basisu_miniz.h
    ktx/basisu/encoder/basisu_opencl.cpp ktx/basisu/encoder/basisu_opencl.h
    ktx/basisu/encoder/basisu_pvrtc1_4.cpp ktx/basisu/encoder/basisu_pvrtc1_4.h
    ktx/basisu/encoder/basisu_resample_filters.cpp ktx/basisu/encoder/basisu_resampler_filters.h
    ktx/basisu/encoder/basisu_resampler.cpp ktx/basisu/encoder/basisu_resampler.h
    ktx/basisu/encoder/basisu_ssim.cpp ktx/basisu/encoder/basisu_ssim.h
    ktx/basisu/encoder/basisu_uastc_enc.cpp ktx/basisu/encoder/basisu_uastc_enc.h
    ktx/basisu/encoder/cppspmd_flow.h ktx/basisu/encoder/cppspmd_math.h
    ktx/basisu/encoder/cppspmd_math_declares.h ktx/basisu/encoder/cppspmd_sse.h
    ktx/basisu/encoder/cppspmd_type_aliases.h
)

SET(MARL_SOURCE_FILES
    marl/debug.cpp marl/memory.cpp marl/scheduler.cpp marl/thread.cpp marl/trace.cpp)
IF(NOT MSVC)
    SET(MARL_SOURCE_FILES ${MARL_SOURCE_FILES}
        marl/osfiber_aarch64.c marl/osfiber_arm.c
        marl/osfiber_asm_aarch64.S marl/osfiber_asm_arm.S
        marl/osfiber_asm_loongarch64.S marl/osfiber_asm_mips64.S
        marl/osfiber_asm_ppc64.S marl/osfiber_asm_rv64.S
        marl/osfiber_asm_x64.S marl/osfiber_asm_x86.S
        marl/osfiber_loongarch64.c marl/osfiber_mips64.c
        marl/osfiber_ppc64.c marl/osfiber_rv64.c
        marl/osfiber_x64.c marl/osfiber_x86.c
        marl/osfiber_emscripten.cpp
    )
    SET_SOURCE_FILES_PROPERTIES(
        marl/osfiber_asm_aarch64.S marl/osfiber_asm_arm.S
        marl/osfiber_asm_loongarch64.S marl/osfiber_asm_mips64.S
        marl/osfiber_asm_ppc64.S marl/osfiber_asm_x64.S marl/osfiber_asm_x86.S
        PROPERTIES LANGUAGE C
    )
ENDIF(NOT MSVC)

SET(GAINPUT_SOURCE_FILES
    gainput/gainput.cpp gainput/gainput.h gainput/GainputInputManager.h
    gainput/GainputInputDevice.h gainput/GainputInputListener.h
    gainput/GainputAllocator.cpp gainput/GainputAllocator.h
    gainput/GainputInputDeltaState.cpp gainput/GainputInputDevice.cpp
    gainput/GainputInputManager.cpp gainput/GainputInputMap.cpp
    gainput/GainputInputState.cpp gainput/GainputMapFilters.cpp
    gainput/builtin/GainputInputDeviceBuiltIn.cpp gainput/dev/GainputNetListener.cpp
    gainput/dev/GainputDev.cpp gainput/dev/GainputMemoryStream.cpp
    gainput/dev/GainputNetAddress.cpp gainput/dev/GainputNetConnection.cpp
    gainput/gestures/GainputButtonStickGesture.cpp gainput/gestures/GainputDoubleClickGesture.cpp
    gainput/gestures/GainputHoldGesture.cpp gainput/gestures/GainputPinchGesture.cpp
    gainput/gestures/GainputRotateGesture.cpp gainput/gestures/GainputSimultaneouslyDownGesture.cpp
    gainput/gestures/GainputTapGesture.cpp gainput/gestures/GainputGestures.h
    gainput/keyboard/GainputInputDeviceKeyboard.cpp gainput/keyboard/GainputKeyboardKeyNames.h
    gainput/mouse/GainputInputDeviceMouse.cpp gainput/mouse/GainputMouseInfo.h
    gainput/pad/GainputInputDevicePad.cpp gainput/pad/GainputInputDevicePadMac.cpp
    gainput/recorder/GainputInputPlayer.cpp gainput/recorder/GainputInputRecorder.cpp
    gainput/recorder/GainputInputRecording.cpp gainput/touch/GainputInputDeviceTouch.cpp
)
IF(APPLE)
    SET(GAINPUT_SOURCE_FILES ${GAINPUT_SOURCE_FILES}
        gainput/GainputIos.mm gainput/GainputMac.mm
        gainput/builtin/GainputInputDeviceBuiltInIos.mm
        gainput/pad/GainputInputDevicePadIos.mm
    )
ENDIF(APPLE)

SET(RECAST_SOURCE_FILES
    recastnavigation/Detour/DetourAlloc.cpp recastnavigation/Detour/DetourAssert.cpp
    recastnavigation/Detour/DetourNode.cpp recastnavigation/Detour/DetourNavMesh.cpp
    recastnavigation/Detour/DetourNavMeshBuilder.cpp recastnavigation/Detour/DetourNavMeshQuery.cpp
    recastnavigation/Detour/DetourCommon.cpp recastnavigation/Detour/DetourCommon.h
    recastnavigation/DetourCrowd/DetourLocalBoundary.cpp recastnavigation/DetourCrowd/DetourObstacleAvoidance.cpp
    recastnavigation/DetourCrowd/DetourPathCorridor.cpp recastnavigation/DetourCrowd/DetourPathQueue.cpp
    recastnavigation/DetourCrowd/DetourProximityGrid.cpp
    recastnavigation/DetourCrowd/DetourCrowd.cpp recastnavigation/DetourCrowd/DetourCrowd.h
    recastnavigation/DetourTileCache/DetourTileCache.cpp recastnavigation/DetourTileCache/DetourTileCache.h
    recastnavigation/DetourTileCache/DetourTileCacheBuilder.cpp
    recastnavigation/Recast/RecastAlloc.cpp recastnavigation/Recast/RecastArea.cpp
    recastnavigation/Recast/RecastAssert.cpp recastnavigation/Recast/RecastContour.cpp
    recastnavigation/Recast/RecastFilter.cpp recastnavigation/Recast/RecastLayers.cpp
    recastnavigation/Recast/RecastMesh.cpp recastnavigation/Recast/RecastMeshDetail.cpp
    recastnavigation/Recast/RecastRasterization.cpp recastnavigation/Recast/RecastRegion.cpp
    recastnavigation/Recast/Recast.cpp recastnavigation/Recast/Recast.h
)

SET(LIBSPATIALINDEX_SOURCE_FILES
    spatialindex/capi/BoundsQuery.cc spatialindex/capi/CountVisitor.cc
    spatialindex/capi/CustomStorage.cc spatialindex/capi/DataStream.cc
    spatialindex/capi/Error.cc spatialindex/capi/IdVisitor.cc
    spatialindex/capi/Index.cc spatialindex/capi/LeafQuery.cc
    spatialindex/capi/ObjVisitor.cc spatialindex/capi/sidx_api.cc
    spatialindex/capi/Utility.cc spatialindex/spatialindex/Ball.cc
    spatialindex/spatialindex/LineSegment.cc spatialindex/spatialindex/MovingPoint.cc
    spatialindex/spatialindex/MovingRegion.cc spatialindex/spatialindex/Point.cc
    spatialindex/spatialindex/Region.cc spatialindex/spatialindex/SpatialIndexImpl.cc
    spatialindex/spatialindex/TimePoint.cc spatialindex/spatialindex/TimeRegion.cc
    spatialindex/mvrtree/Index.cc spatialindex/mvrtree/Index.h spatialindex/mvrtree/Leaf.cc spatialindex/mvrtree/Leaf.h
    spatialindex/mvrtree/MVRTree.cc spatialindex/mvrtree/MVRTree.h
    spatialindex/mvrtree/Node.cc spatialindex/mvrtree/Node.h spatialindex/mvrtree/PointerPoolNode.h
    spatialindex/mvrtree/Statistics.cc spatialindex/mvrtree/Statistics.h
    spatialindex/rtree/BulkLoader.cc spatialindex/rtree/BulkLoader.h spatialindex/rtree/Index.cc spatialindex/rtree/Index.h
    spatialindex/rtree/Leaf.cc spatialindex/rtree/Leaf.h spatialindex/rtree/RTree.cc spatialindex/rtree/RTree.h
    spatialindex/rtree/Node.cc spatialindex/rtree/Node.h spatialindex/rtree/PointerPoolNode.h
    spatialindex/rtree/Statistics.cc spatialindex/rtree/Statistics.h
    spatialindex/storagemanager/DiskStorageManager.cc spatialindex/storagemanager/DiskStorageManager.h
    spatialindex/storagemanager/MemoryStorageManager.cc spatialindex/storagemanager/MemoryStorageManager.h
    spatialindex/storagemanager/RandomEvictionsBuffer.cc spatialindex/storagemanager/RandomEvictionsBuffer.h
    spatialindex/storagemanager/Buffer.cc spatialindex/Ball.h spatialindex/LineSegment.h
    spatialindex/MovingPoint.h spatialindex/MVRTree.h spatialindex/Point.h spatialindex/Region.h
    spatialindex/RTree.h spatialindex/SpatialIndex.h spatialindex/TimePoint.h spatialindex/TimeRegion.h
    spatialindex/TPRTree.h spatialindex/Version.h spatialindex/tools/rand48.cc spatialindex/tools/Tools.cc
)

SET(BLEND2D_SOURCE_FILES
    blend2d/blend2d.h blend2d/blend2d-debug.h blend2d/blend2d-impl.h blend2d/api.h blend2d/api-build_p.h blend2d/api-build_test_p.h
    blend2d/api-globals.cpp blend2d/api-impl.h blend2d/api-internal_p.h blend2d/api-nocxx.cpp blend2d/array.cpp blend2d/array_test.cpp
    blend2d/array.h blend2d/array_p.h blend2d/bitarray.cpp blend2d/bitarray_test.cpp blend2d/bitarray.h blend2d/bitarray_p.h
    blend2d/bitset.cpp blend2d/bitset_test.cpp blend2d/bitset.h blend2d/bitset_p.h blend2d/compop.cpp blend2d/compop_p.h
    blend2d/context.cpp blend2d/context_test.cpp blend2d/context.h blend2d/context_p.h blend2d/fontdata.cpp blend2d/fontdata.h
    blend2d/filesystem.cpp blend2d/filesystem.h blend2d/filesystem_p.h blend2d/font.cpp blend2d/font.h blend2d/font_p.h
    blend2d/font_test.cpp blend2d/fontdata_p.h blend2d/fontdefs.h blend2d/fontface.cpp blend2d/fontface.h blend2d/fontface_p.h
    blend2d/fontfeaturesettings.cpp blend2d/fontfeaturesettings_test.cpp blend2d/fontfeaturesettings.h blend2d/fontfeaturesettings_p.h
    blend2d/fontmanager.cpp blend2d/fontmanager.h blend2d/fontmanager_p.h blend2d/fonttagset.cpp blend2d/fonttagset_p.h
    blend2d/fonttagdata_p.h blend2d/fonttagdataids.cpp blend2d/fonttagdataids_test.cpp blend2d/fonttagdataids_p.h blend2d/format_p.h
    blend2d/fonttagdatainfo.cpp blend2d/fonttagdatainfo_test.cpp blend2d/fonttagdatainfo_p.h blend2d/format.cpp blend2d/format.h
    blend2d/fontvariationsettings.cpp blend2d/fontvariationsettings_test.cpp blend2d/fontvariationsettings.h blend2d/fontvariationsettings_p.h
    blend2d/geometry.cpp blend2d/geometry.h blend2d/geometry_p.h blend2d/glyphbuffer.cpp blend2d/glyphbuffer.h blend2d/glyphbuffer_p.h
    blend2d/glyphrun.h blend2d/gradient.cpp blend2d/gradient_test.cpp blend2d/gradient.h blend2d/gradient_p.h blend2d/imagecodec_test.cpp
    blend2d/image.cpp blend2d/image_test.cpp blend2d/image.h blend2d/image_p.h blend2d/imagecodec.cpp blend2d/imagecodec.h
    blend2d/imagedecoder.cpp blend2d/imagedecoder.h blend2d/imageencoder.cpp blend2d/imageencoder.h blend2d/imagescale.cpp blend2d/imagescale_p.h
    blend2d/math.cpp blend2d/math_test.cpp blend2d/math_p.h blend2d/matrix.cpp blend2d/matrix_avx.cpp blend2d/matrix_sse2.cpp
    blend2d/matrix_test.cpp blend2d/matrix.h blend2d/matrix_p.h blend2d/object.cpp blend2d/object.h blend2d/object_p.h
    blend2d/path.cpp blend2d/path_test.cpp blend2d/path.h blend2d/path_p.h blend2d/pathstroke.cpp blend2d/pathstroke_p.h
    blend2d/pattern.cpp blend2d/pattern.h blend2d/pattern_p.h blend2d/runtime.cpp blend2d/runtime.h blend2d/runtime_p.h
    blend2d/pixelconverter.cpp blend2d/pixelconverter_avx2.cpp blend2d/pixelconverter_sse2.cpp blend2d/pixelconverter_ssse3.cpp
    blend2d/pixelconverter_test.cpp blend2d/pixelconverter.h blend2d/pixelconverter_p.h blend2d/trace.cpp blend2d/trace_p.h
    blend2d/random.cpp blend2d/random_test.cpp blend2d/random.h blend2d/random_p.h blend2d/rgba_test.cpp blend2d/rgba.h
    blend2d/string.cpp blend2d/string_test.cpp blend2d/string.h blend2d/string_p.h
    blend2d/var.cpp blend2d/var_test.cpp blend2d/var.h blend2d/var_p.h

    blend2d/codec/bmpcodec.cpp blend2d/codec/bmpcodec_p.h blend2d/codec/jpegcodec.cpp blend2d/codec/jpegcodec_p.h
    blend2d/codec/jpeghuffman.cpp blend2d/codec/jpeghuffman_p.h blend2d/codec/jpegops.cpp blend2d/codec/jpegops_sse2.cpp
    blend2d/codec/jpegops_p.h blend2d/codec/pngcodec.cpp blend2d/codec/pngcodec_p.h
    blend2d/codec/pngops.cpp blend2d/codec/pngops_sse2.cpp blend2d/codec/pngops_p.h

    blend2d/compression/checksum.cpp blend2d/compression/checksum_test.cpp blend2d/compression/checksum_p.h
    blend2d/compression/deflatedecoder.cpp blend2d/compression/deflatedecoder_p.h blend2d/compression/deflatedefs_p.h
    blend2d/compression/deflateencoder.cpp blend2d/compression/deflateencoder_p.h blend2d/compression/matchfinder_p.h

    blend2d/opentype/otcff.cpp blend2d/opentype/otcff_test.cpp blend2d/opentype/otcff_p.h
    blend2d/opentype/otcmap.cpp blend2d/opentype/otcmap_p.h blend2d/opentype/otcore.cpp blend2d/opentype/otcore_p.h
    blend2d/opentype/otdefs_p.h blend2d/opentype/otface.cpp blend2d/opentype/otface_p.h
    blend2d/opentype/otglyf.cpp blend2d/opentype/otglyf_avx2.cpp blend2d/opentype/otglyf_sse4_2.cpp
    blend2d/opentype/otglyf_p.h blend2d/opentype/otkern.cpp blend2d/opentype/otkern_p.h
    blend2d/opentype/otlayout.cpp blend2d/opentype/otlayout_p.h blend2d/opentype/otlayoutcontext_p.h
    blend2d/opentype/otlayouttables_p.h blend2d/opentype/otmetrics.cpp blend2d/opentype/otmetrics_p.h
    blend2d/opentype/otname.cpp blend2d/opentype/otname_p.h blend2d/opentype/otplatform_p.h

    blend2d/pipeline/pipedefs.cpp blend2d/pipeline/pipedefs_p.h blend2d/pipeline/piperuntime.cpp blend2d/pipeline/piperuntime_p.h
    blend2d/pipeline/jit/compoppart.cpp blend2d/pipeline/jit/compoppart_p.h
    blend2d/pipeline/jit/fetchgradientpart.cpp blend2d/pipeline/jit/fetchgradientpart_p.h
    blend2d/pipeline/jit/fetchpart.cpp blend2d/pipeline/jit/fetchpart_p.h
    blend2d/pipeline/jit/fetchpatternpart.cpp blend2d/pipeline/jit/fetchpatternpart_p.h
    blend2d/pipeline/jit/fetchpixelptrpart.cpp blend2d/pipeline/jit/fetchpixelptrpart_p.h
    blend2d/pipeline/jit/fetchsolidpart.cpp blend2d/pipeline/jit/fetchsolidpart_p.h
    blend2d/pipeline/jit/fetchutils.cpp blend2d/pipeline/jit/fetchutils_p.h
    blend2d/pipeline/jit/fillpart.cpp blend2d/pipeline/jit/fillpart_p.h blend2d/pipeline/jit/jitbase_p.h
    blend2d/pipeline/jit/pipecompiler.cpp blend2d/pipeline/jit/pipecompiler_p.h blend2d/pipeline/jit/pipedebug_p.h
    blend2d/pipeline/jit/pipegencore.cpp blend2d/pipeline/jit/pipegencore_p.h
    blend2d/pipeline/jit/pipegenruntime_p.h blend2d/pipeline/jit/pipegenruntime.cpp
    blend2d/pipeline/jit/pipepart.cpp blend2d/pipeline/jit/pipepart_p.h

    blend2d/pipeline/reference/compopgeneric_p.h blend2d/pipeline/reference/fetchgeneric_p.h blend2d/pipeline/reference/fillgeneric_p.h
    blend2d/pipeline/reference/fixedpiperuntime_p.h blend2d/pipeline/reference/fixedpiperuntime.cpp
    blend2d/pipeline/reference/pixelbufferptr_p.h blend2d/pipeline/reference/pixelgeneric_p.h

    blend2d/pixelops/interpolation.cpp blend2d/pixelops/interpolation_avx2.cpp blend2d/pixelops/interpolation_sse2.cpp
    blend2d/pixelops/funcs.cpp blend2d/pixelops/funcs_p.h blend2d/pixelops/scalar_test.cpp blend2d/pixelops/scalar_p.h

    blend2d/raster/analyticrasterizer_test.cpp blend2d/raster/analyticrasterizer_p.h
    blend2d/raster/debugging_p.h blend2d/raster/edgebuilder_p.h blend2d/raster/edgestorage_p.h
    blend2d/raster/rastercontext.cpp blend2d/raster/rastercontext_p.h
    blend2d/raster/rastercontextops.cpp blend2d/raster/rastercontextops_p.h
    blend2d/raster/rasterdefs_p.h blend2d/raster/renderbatch_p.h blend2d/raster/rendercommand_p.h
    blend2d/raster/rendercommandprocasync_p.h blend2d/raster/rendercommandprocsync_p.h
    blend2d/raster/renderfetchdata.cpp blend2d/raster/renderfetchdata_p.h
    blend2d/raster/renderjob_p.h blend2d/raster/renderjobproc_p.h blend2d/raster/renderqueue_p.h
    blend2d/raster/rendertargetinfo.cpp blend2d/raster/rendertargetinfo_p.h blend2d/raster/statedata_p.h blend2d/raster/styledata_p.h
    blend2d/raster/workdata.cpp blend2d/raster/workdata_p.h blend2d/raster/workermanager.cpp blend2d/raster/workermanager_p.h
    blend2d/raster/workerproc.cpp blend2d/raster/workerproc_p.h blend2d/raster/workersynchronization.cpp blend2d/raster/workersynchronization_p.h

    blend2d/simd/simd_p.h blend2d/simd/simd_test.cpp blend2d/simd/simd_test_p.h
    blend2d/simd/simdbase_p.h blend2d/simd/simdarm_p.h blend2d/simd/simdarm_test_asimd.cpp
    blend2d/simd/simdx86_p.h blend2d/simd/simdx86_test_avx.cpp blend2d/simd/simdx86_test_avx2.cpp
    blend2d/simd/simdx86_test_avx512.cpp blend2d/simd/simdx86_test_sse2.cpp blend2d/simd/simdx86_test_sse4_1.cpp
    blend2d/simd/simdx86_test_sse4_2.cpp blend2d/simd/simdx86_test_ssse3.cpp

    blend2d/support/algorithm_test.cpp blend2d/support/algorithm_p.h
    blend2d/support/arenaallocator.cpp blend2d/support/arenaallocator_p.h
    blend2d/support/arenabitarray_test.cpp blend2d/support/arenabitarray_p.h
    blend2d/support/arenahashmap.cpp blend2d/support/arenahashmap_test.cpp blend2d/support/arenahashmap_p.h
    blend2d/support/arenalist_test.cpp blend2d/support/arenalist_p.h blend2d/support/arenatree_test.cpp
    blend2d/support/arenatree_p.h blend2d/support/bitops_test.cpp blend2d/support/bitops_p.h
    blend2d/support/fixedbitarray_p.h blend2d/support/hashops_p.h blend2d/support/intops_test.cpp  blend2d/support/intops_p.h
    blend2d/support/lookuptable_p.h blend2d/support/memops_test.cpp  blend2d/support/memops_p.h
    blend2d/support/ptrops_test.cpp blend2d/support/ptrops_p.h blend2d/support/stringops_p.h blend2d/support/wrap_p.h
    blend2d/support/scopedallocator.cpp blend2d/support/scopedallocator_p.h blend2d/support/scopedbuffer_p.h
    blend2d/support/zeroallocator.cpp blend2d/support/zeroallocator_test.cpp blend2d/support/zeroallocator_p.h

    blend2d/tables/tables.cpp blend2d/tables/tables_test.cpp blend2d/tables/tables_p.h
    blend2d/threading/atomic_p.h blend2d/threading/conditionvariable_p.h
    blend2d/threading/futex.cpp blend2d/threading/futex_p.h blend2d/threading/mutex_p.h
    blend2d/threading/thread.cpp blend2d/threading/thread_p.h blend2d/threading/threadingutils_p.h
    blend2d/threading/threadpool.cpp blend2d/threading/threadpool_test.cpp blend2d/threading/threadpool_p.h
    blend2d/threading/uniqueidgenerator.cpp blend2d/threading/uniqueidgenerator_p.h
    blend2d/unicode/unicode.cpp blend2d/unicode/unicode_test.cpp blend2d/unicode/unicode_p.h
)

SET(TWEENY_SOURCE_FILES
    tweeny/tween.h tweeny/tween.tcc tweeny/tweeny.h tweeny/tweeny.tcc
    tweeny/tweenone.tcc tweeny/tweenpoint.tcc
)

SET(QUADRIFLOW_SOURCE_FILES
    quadriflow/manifold/types.h quadriflow/manifold/GridIndex.h
    quadriflow/manifold/Intersection.h quadriflow/manifold/Intersection.cc
    quadriflow/manifold/Manifold.h quadriflow/manifold/Manifold.cc
    quadriflow/manifold/MeshProjector.h quadriflow/manifold/MeshProjector.cc
    quadriflow/manifold/Octree.h quadriflow/manifold/Octree.cc
    quadriflow/manifold/Parser.h quadriflow/manifold/Parser.cc
)

SOURCE_GROUP("LevelDB Files" FILES ${LEVELDB_SOURCE_FILES})
SOURCE_GROUP("ApproxMVBB Files" FILES ${APPROXMVBB_SOURCE_FILES})
SOURCE_GROUP("NormalMap Files" FILES ${NORMALMAP_SOURCE_FILES})
SOURCE_GROUP("LasZIP Files" FILES ${LASZIP_SOURCE_FILES})
SOURCE_GROUP("OzzAnimation Files" FILES ${OZZ_SOURCE_FILES})
SOURCE_GROUP("PMP & MCut Files" FILES ${PMP_MCUT_SOURCE_FILES})
SOURCE_GROUP("ImGUI Files" FILES ${IMGUI_SOURCE_FILES})
SOURCE_GROUP("PinYin-IME Files" FILES ${PINYIN_SOURCE_FILES})
SOURCE_GROUP("TinySpline Files" FILES ${TINYSPLINE_SOURCE_FILES})
SOURCE_GROUP("MbedTLS Files" FILES ${MBEDTLS_SOURCE_FILES})
SOURCE_GROUP("LibHV Files" FILES ${LIBHV_SOURCE_FILES})
SOURCE_GROUP("KTX Files" FILES ${KTX_SOURCE_FILES})
SOURCE_GROUP("Marl Files" FILES ${MARL_SOURCE_FILES})
SOURCE_GROUP("GaInput Files" FILES ${GAINPUT_SOURCE_FILES})
SOURCE_GROUP("RecastNavigation Files" FILES ${RECAST_SOURCE_FILES})
SOURCE_GROUP("libSpatialIndex Files" FILES ${LIBSPATIALINDEX_SOURCE_FILES})
SOURCE_GROUP("Blend2D Files" FILES ${BLEND2D_SOURCE_FILES})
SOURCE_GROUP("Tweeny Files" FILES ${TWEENY_SOURCE_FILES})
SOURCE_GROUP("QuadriFlow Files" FILES ${QUADRIFLOW_SOURCE_FILES})
#######################

SET(LIB_NAME osgVerseDependency)
SET(LIBRARY_FILES
	tiny_obj_loader.h tiny_gltf.h picojson.h nanoflann.hpp exprtk.hpp mio.hpp any.hpp
    backward.hpp strtk.hpp ghc/filesystem.hpp rapidxml/rapidxml.hpp rapidjson/rapidjson.h
    nanoid/nanoid.cpp nanoid/nanoid.h nanoid/crypto_random.cpp nanoid/crypto_random.h
    sqlite3.c sqlite3.h miniz.c miniz.h ofbx.cpp ofbx.h tinyexr.cc tinyexr.h dkm_parallel.hpp
    mikktspace.c mikktspace.h laplacian_deformation.cpp laplacian_deformation.hpp
	mimalloc/static.c mimalloc/mimalloc.h mimalloc/mimalloc-new-delete.h lsqcpp.hpp
	lightmapper.h xatlas.cpp xatlas.h #microprofile.cpp microprofile.h
    GL/glew.c GL/glew.h xxYUV/rgb2yuv.cpp xxYUV/yuv2rgb.cpp xxYUV/cpu.cpp
    BSplineFitting/cubic_b_spline.cpp BSplineFitting/open_cubic_b_spline.cpp BSplineFitting/spline_curve_fitting.cpp
    ${LEVELDB_SOURCE_FILES} ${APPROXMVBB_SOURCE_FILES} ${NORMALMAP_SOURCE_FILES}
    ${PMP_MCUT_SOURCE_FILES} ${OZZ_SOURCE_FILES} ${LASZIP_SOURCE_FILES} ${IMGUI_SOURCE_FILES}
    ${PINYIN_SOURCE_FILES} ${TINYSPLINE_SOURCE_FILES} ${MBEDTLS_SOURCE_FILES} ${LIBHV_SOURCE_FILES}
    ${KTX_SOURCE_FILES} ${MARL_SOURCE_FILES} ${GAINPUT_SOURCE_FILES} ${RECAST_SOURCE_FILES}
    ${LIBSPATIALINDEX_SOURCE_FILES} ${BLEND2D_SOURCE_FILES} ${TWEENY_SOURCE_FILES}
)

IF(LIBIGL_FOUND)
    SET(LIBRARY_FILES ${LIBRARY_FILES} ${QUADRIFLOW_SOURCE_FILES})
ENDIF(LIBIGL_FOUND)

ADD_DEFINITIONS(-DApproxMVBB_BUILD_LIBRARY -DGLEW_STATIC -DGLEW_NO_GLU -DLASZIPDLL_EXPORTS -DBL_STATIC -DBL_BUILD_NO_JIT
                -DLEVELDB_COMPILE_LIBRARY -DLEVELDB_HAS_PORT_CONFIG_H=1 -DHV_STATICLIB -DWITH_MBEDTLS
                -DKHRONOS_STATIC -DLIBKTX -DKTX_FEATURE_KTX1 -DKTX_FEATURE_WRITE=1 -DKTX_OMIT_VULKAN=1
                -DBASISD_SUPPORT_KTX2_ZSTD=0 -DBASISD_SUPPORT_KTX2=1 -DBASISU_SUPPORT_OPENCL=0
				-DIMGUI_IMPL_OPENGL_LOADER_GLEW -DUNORDERED -DHAVE_UNORDERED_MAP=1)
IF(MSVC_VERSION LESS 1910)
    ADD_DEFINITIONS(-DWITHOUT_HTTP_CONTENT)  # disable use of json.hpp in VS2015 or lower
ENDIF(MSVC_VERSION LESS 1910)

IF(OPENMP_FOUND)
    ADD_DEFINITIONS(-DWITH_OMP)
ENDIF(OPENMP_FOUND)

IF("sse4.2" IN_LIST VERSE_SIMD_FEATURES)
    ADD_DEFINITIONS(-DBL_BUILD_OPT_SSE4_2 -DBASISU_SUPPORT_SSE)
ELSEIF("sse4.1" IN_LIST VERSE_SIMD_FEATURES)
    ADD_DEFINITIONS(-DBL_BUILD_OPT_SSE4_1 -DBASISU_SUPPORT_SSE)
ELSEIF("ssse3" IN_LIST VERSE_SIMD_FEATURES)
    ADD_DEFINITIONS(-DBL_BUILD_OPT_SSSE3 -DBASISU_SUPPORT_SSE)
ELSEIF("sse3" IN_LIST VERSE_SIMD_FEATURES)
    ADD_DEFINITIONS(-DBL_BUILD_OPT_SSE3 -DBASISU_SUPPORT_SSE)
ELSEIF("sse2" IN_LIST VERSE_SIMD_FEATURES)
    ADD_DEFINITIONS(-DBL_BUILD_OPT_SSE2)#-DBASISU_SUPPORT_SSE)
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

INCLUDE_DIRECTORIES(libhv libhv/all libhv/mbedtls imgui stb ozz leveldb ktx ktx/basisu/zstd ktx/dfdutils
                    quadriflow recastnavigation/Detour "${CMAKE_BINARY_DIR}/3rdparty/leveldb")
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

SET(THIRDPARTY_INCLUDE_FILES any.hpp strtk.hpp backward.hpp)
SET(IMGUI_INCLUDE_FILES imgui/imgui.h imgui/imconfig.h imgui/ImGuizmo.h)
INSTALL(FILES ${THIRDPARTY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse COMPONENT libosgverse-dev)
INSTALL(FILES ${IMGUI_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/imgui COMPONENT libosgverse-dev)
