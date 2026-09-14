SET(CMAKE_SHARED_LIBRARY_PREFIX "")
IF(UNIX)
    SET(INSTALL_LIBDIR "lib/osgPlugins-${OSG_MAJOR_VERSION}.${OSG_MINOR_VERSION}.${OSG_PATCH_VERSION}")
ENDIF(UNIX)

# meshes
ADD_SUBDIRECTORY(osgdb_3dtiles)
ADD_SUBDIRECTORY(osgdb_fbx)
ADD_SUBDIRECTORY(osgdb_gltf)
ADD_SUBDIRECTORY(osgdb_mesh)
ADD_SUBDIRECTORY(osgdb_terrain)
ADD_SUBDIRECTORY(osgdb_tms)

# point clouds & 3DGS
ADD_SUBDIRECTORY(osgdb_3dgs)
ADD_SUBDIRECTORY(osgdb_ept)
ADD_SUBDIRECTORY(osgdb_potree)
ADD_SUBDIRECTORY(osgdb_vdb)

# vectors
IF(MSVC AND MSVC_VERSION LESS 1909)
    # Disable geojson & s57 plugins in VS2015
ELSE()
    ADD_SUBDIRECTORY(osgdb_geojson)
    ADD_SUBDIRECTORY(osgdb_kml)
    ADD_SUBDIRECTORY(osgdb_s57)
ENDIF()
ADD_SUBDIRECTORY(osgdb_mbtiles)
ADD_SUBDIRECTORY(osgdb_mvt)
ADD_SUBDIRECTORY(osgdb_mlt)
ADD_SUBDIRECTORY(osgdb_osm)
ADD_SUBDIRECTORY(osgdb_shp)

# images
ADD_SUBDIRECTORY(osgdb_image)
ADD_SUBDIRECTORY(osgdb_ktx)
ADD_SUBDIRECTORY(osgdb_netcdf)
ADD_SUBDIRECTORY(osgdb_tiff)
ADD_SUBDIRECTORY(osgdb_webp)

# video demuxer / muxer
ADD_SUBDIRECTORY(osgdb_ffmpeg)
ADD_SUBDIRECTORY(osgdb_mediastream)
ADD_SUBDIRECTORY(osgdb_video)

# video decoder / encoder
ADD_SUBDIRECTORY(osgdb_nvcodec)
ADD_SUBDIRECTORY(osgdb_vacodec)

# vision devices
ADD_SUBDIRECTORY(osgdb_inudev)
ADD_SUBDIRECTORY(osgdb_webcam)

# physics
ADD_SUBDIRECTORY(osgdb_bullet)

# database
ADD_SUBDIRECTORY(osgdb_leveldb)
ADD_SUBDIRECTORY(osgdb_odbc)

# misc
ADD_SUBDIRECTORY(osgdb_web)
ADD_SUBDIRECTORY(osgdb_verse)
ADD_SUBDIRECTORY(osgdb_pbrlayout)
