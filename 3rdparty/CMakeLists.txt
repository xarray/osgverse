CONFIGURE_FILE("leveldb/port/port_config.h.in"
               "${CMAKE_BINARY_DIR}/3rdparty/leveldb/port/port_config.h")
SET(LEVELDB_SOURCE_FILES
    "leveldb/db/builder.cc" "leveldb/db/builder.h" "leveldb/db/c.cc"
    "leveldb/db/db_impl.cc" "leveldb/db/db_impl.h" "leveldb/db/db_iter.cc" "leveldb/db/db_iter.h"
    "leveldb/db/dbformat.cc" "leveldb/db/dbformat.h" "leveldb/db/dumpfile.cc"
    "leveldb/db/filename.cc" "leveldb/db/filename.h" "leveldb/db/log_format.h"
    "leveldb/db/log_reader.cc" "leveldb/db/log_reader.h" "leveldb/db/log_writer.cc" "leveldb/db/log_writer.h"
    "leveldb/db/memtable.cc" "leveldb/db/memtable.h" "leveldb/db/repair.cc"
    "leveldb/db/skiplist.h" "leveldb/db/snapshot.h" "leveldb/db/table_cache.cc" "leveldb/db/table_cache.h"
    "leveldb/db/version_edit.cc" "leveldb/db/version_edit.h" "leveldb/db/version_set.cc" "leveldb/db/version_set.h"
    "leveldb/db/write_batch_internal.h" "leveldb/db/write_batch.cc"
    "leveldb/port/port_stdcxx.h" "leveldb/port/port.h" "leveldb/port/thread_annotations.h"
    "leveldb/table/block_builder.cc" "leveldb/table/block_builder.h"
    "leveldb/table/block.cc" "leveldb/table/block.h" "leveldb/table/filter_block.cc" "leveldb/table/filter_block.h"
    "leveldb/table/format.cc" "leveldb/table/format.h" "leveldb/table/iterator_wrapper.h" "leveldb/table/iterator.cc"
    "leveldb/table/merger.cc" "leveldb/table/merger.h" "leveldb/table/table_builder.cc" "leveldb/table/table.cc"
    "leveldb/table/two_level_iterator.cc" "leveldb/table/two_level_iterator.h"
    "leveldb/util/arena.cc" "leveldb/util/arena.h" "leveldb/util/bloom.cc" "leveldb/util/cache.cc" "leveldb/util/coding.cc"
    "leveldb/util/coding.h" "leveldb/util/comparator.cc" "leveldb/util/crc32c.cc" "leveldb/util/crc32c.h" "leveldb/util/env.cc"
    "leveldb/util/filter_policy.cc" "leveldb/util/hash.cc" "leveldb/util/hash.h"
    "leveldb/util/logging.cc" "leveldb/util/logging.h" "leveldb/util/mutexlock.h" "leveldb/util/no_destructor.h"
    "leveldb/util/options.cc" "leveldb/util/random.h" "leveldb/util/status.cc"
    "leveldb/helpers/memenv/memenv.cc" "leveldb/helpers/memenv/memenv.h"
)
IF(WIN32)
    SET(LEVELDB_SOURCE_FILES ${LEVELDB_SOURCE_FILES} "leveldb/util/env_windows.cc" "leveldb/util/windows_logger.h")
ELSE(WIN32)
    SET(LEVELDB_SOURCE_FILES ${LEVELDB_SOURCE_FILES} "leveldb/util/env_posix.cc" "leveldb/util/posix_logger.h")
ENDIF(WIN32)

SET(ApproxMVBB_VERSION_MAJOR 3)
SET(ApproxMVBB_VERSION_MINOR 1)
SET(ApproxMVBB_VERSION_PATCH 2)
SET(ApproxMVBB_FORCE_MSGLOG_LEVEL 0)
CONFIGURE_FILE(ApproxMVBB/GeometryPredicates/Config.hpp.in.cmake "${CMAKE_BINARY_DIR}/3rdparty/ApproxMVBB/GeometryPredicates/Config.hpp")
CONFIGURE_FILE(ApproxMVBB/Config/Config.hpp.in.cmake "${CMAKE_BINARY_DIR}/3rdparty/ApproxMVBB/Config/Config.hpp")
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
    pmp/SurfaceMesh.cpp pmp/SurfaceMesh.h #pmp/SurfaceMeshIO.cpp pmp/SurfaceMeshIO.h
    pmp/algorithms/Heap.h pmp/algorithms/NormalCone.h pmp/algorithms/Quadric.h
    pmp/algorithms/BarycentricCoordinates.h
    pmp/algorithms/DifferentialGeometry.cpp pmp/algorithms/DifferentialGeometry.h
    pmp/algorithms/DistancePointTriangle.cpp pmp/algorithms/DistancePointTriangle.h
    pmp/algorithms/SurfaceCurvature.cpp pmp/algorithms/SurfaceCurvature.h
    pmp/algorithms/SurfaceFactory.cpp pmp/algorithms/SurfaceFactory.h
    pmp/algorithms/SurfaceFairing.cpp pmp/algorithms/SurfaceFairing.h
    pmp/algorithms/SurfaceFeatures.cpp pmp/algorithms/SurfaceFeatures.h
    pmp/algorithms/SurfaceGeodesic.cpp pmp/algorithms/SurfaceGeodesic.h
    pmp/algorithms/SurfaceHoleFilling.cpp pmp/algorithms/SurfaceHoleFilling.h
    pmp/algorithms/SurfaceNormals.cpp pmp/algorithms/SurfaceNormals.h
    pmp/algorithms/SurfaceParameterization.cpp pmp/algorithms/SurfaceParameterization.h
    pmp/algorithms/SurfaceRemeshing.cpp pmp/algorithms/SurfaceRemeshing.h
    pmp/algorithms/SurfaceSimplification.cpp pmp/algorithms/SurfaceSimplification.h
    pmp/algorithms/SurfaceSmoothing.cpp pmp/algorithms/SurfaceSmoothing.h
    pmp/algorithms/SurfaceSubdivision.cpp pmp/algorithms/SurfaceSubdivision.h
    pmp/algorithms/SurfaceTriangulation.cpp pmp/algorithms/SurfaceTriangulation.h
    pmp/algorithms/TriangleKdTree.cpp pmp/algorithms/TriangleKdTree.h
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
	imgui/imgui_impl_opengl2.cpp imgui/imgui_impl_opengl2.h
	imgui/imgui_impl_opengl3.cpp imgui/imgui_impl_opengl3.h
)

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
#######################

SET(LIB_NAME osgVerseDependency)
SET(LIBRARY_FILES
	tiny_obj_loader.h tiny_gltf.h json.hpp picojson.h nanoflann.hpp exprtk.hpp
    sqlite3.c sqlite3.h strtk.hpp tinydir.h mio.hpp rapidxml/rapidxml.hpp
    tinyexr.cc tinyexr.h miniz.c miniz.h ofbx.cpp ofbx.h mikktspace.c mikktspace.h
    laplacian_deformation.cpp laplacian_deformation.hpp GL/glew.c GL/glew.h any.hpp
    nanoid/nanoid.cpp nanoid/nanoid.h nanoid/crypto_random.cpp nanoid/crypto_random.h
    ${LEVELDB_SOURCE_FILES} ${APPROXMVBB_SOURCE_FILES} ${NORMALMAP_SOURCE_FILES}
    ${PMP_MCUT_SOURCE_FILES} ${OZZ_SOURCE_FILES} ${LASZIP_SOURCE_FILES}
    ${IMGUI_SOURCE_FILES} ${PINYIN_SOURCE_FILES} ${TINYSPLINE_SOURCE_FILES}
)

ADD_DEFINITIONS(-DApproxMVBB_BUILD_LIBRARY -DGLEW_STATIC -DGLEW_NO_GLU -DLASZIPDLL_EXPORTS
                -DLEVELDB_COMPILE_LIBRARY -DLEVELDB_PLATFORM_WINDOWS -DLEVELDB_HAS_PORT_CONFIG_H=1
                -DUNORDERED -DHAVE_UNORDERED_MAP=1)
INCLUDE_DIRECTORIES(leveldb ozz imgui "${CMAKE_BINARY_DIR}/3rdparty/leveldb")
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)

SET(THIRDPARTY_INCLUDE_FILES json.hpp any.hpp strtk.hpp)
SET(IMGUI_INCLUDE_FILES imgui/imgui.h imgui/imconfig.h imgui/ImGuizmo.h)
INSTALL(FILES ${THIRDPARTY_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse COMPONENT libosgverse-dev)
INSTALL(FILES ${IMGUI_INCLUDE_FILES} DESTINATION ${INSTALL_INCDIR}/osgVerse/imgui COMPONENT libosgverse-dev)
