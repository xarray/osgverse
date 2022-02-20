SET(LIB_NAME osgVerseDependency)
SET(LIBRARY_FILES
	tinyexr.cc tinyexr.h tiny_gltf.h json.hpp
    tiny_obj_loader.cc tiny_obj_loader.h
    miniz.c miniz.h ofbx.cpp ofbx.h
    mikktspace.c mikktspace.h
    normalmap/boxblur.cpp normalmap/boxblur.h
    normalmap/gaussianblur.cpp normalmap/gaussianblur.h
    normalmap/intensitymap.cpp normalmap/intensitymap.h
    normalmap/normalmapgenerator.cpp normalmap/normalmapgenerator.h
    normalmap/specularmapgenerator.cpp normalmap/specularmapgenerator.h
    normalmap/ssaogenerator.cpp normalmap/ssaogenerator.h
)

NEW_LIBRARY(${LIB_NAME} STATIC)
TARGET_COMPILE_OPTIONS(${LIB_NAME} PUBLIC -D_SCL_SECURE_NO_WARNINGS)
