SET(ApproxMVBB_VERSION_MAJOR 3)
SET(ApproxMVBB_VERSION_MINOR 1)
SET(ApproxMVBB_VERSION_PATCH 2)
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

SET(VHACD_SOURCE_FILES
    VHACD/btAlignedAllocator.cpp VHACD/btAlignedAllocator.h VHACD/btAlignedObjectArray.h
    VHACD/btConvexHullComputer.cpp VHACD/btConvexHullComputer.h VHACD/btMinMax.h
    VHACD/btScalar.h VHACD/btVector3.h VHACD/FloatMath.cpp VHACD/FloatMath.h
    VHACD/FloatMath.inl VHACD/VHACD-ASYNC.cpp VHACD/VHACD.cpp VHACD/VHACD.h
    VHACD/vhacdCircularList.h VHACD/vhacdCircularList.inl VHACD/vhacdICHull.cpp VHACD/vhacdICHull.h
    VHACD/vhacdManifoldMesh.cpp VHACD/vhacdManifoldMesh.h VHACD/vhacdMesh.cpp VHACD/vhacdMesh.h
    VHACD/vhacdMutex.h VHACD/vhacdRaycastMesh.cpp VHACD/vhacdRaycastMesh.h VHACD/vhacdSArray.h
    VHACD/vhacdTimer.h VHACD/vhacdVector.h VHACD/vhacdVector.inl VHACD/vhacdVHACD.h
    VHACD/vhacdVolume.cpp VHACD/vhacdVolume.h
)

SOURCE_GROUP("ApproxMVBB Files" FILES ${APPROXMVBB_SOURCE_FILES})
SOURCE_GROUP("NormalMap Files" FILES ${NORMALMAP_SOURCE_FILES})
SOURCE_GROUP("VHACD Files" FILES ${VHACD_SOURCE_FILES})
#######################

SET(LIB_NAME osgVerseDependency)
SET(LIBRARY_FILES
	tinyexr.cc tinyexr.h tiny_obj_loader.cc tiny_obj_loader.h tiny_gltf.h json.hpp
    miniz.c miniz.h ofbx.cpp ofbx.h mikktspace.c mikktspace.h
    ${APPROXMVBB_SOURCE_FILES} ${NORMALMAP_SOURCE_FILES} ${VHACD_SOURCE_FILES}
)

ADD_DEFINITIONS(-DApproxMVBB_BUILD_LIBRARY)
INCLUDE_DIRECTORIES("${CMAKE_BINARY_DIR}/3rdparty")
NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
