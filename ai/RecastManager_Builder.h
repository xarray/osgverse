#pragma once

#include <osg/Transform>
#include <osg/Geometry>
#include <recastnavigation/Recast/Recast.h>
#include <recastnavigation/Detour/DetourNavMesh.h>
#include <recastnavigation/Detour/DetourNavMeshBuilder.h>
#include <recastnavigation/DetourTileCache/DetourTileCache.h>
#include <recastnavigation/DetourTileCache/DetourTileCacheBuilder.h>
#include <recastnavigation/DetourCrowd/DetourCrowd.h>

namespace osgVerse
{

    struct AreaStub
    {
        osg::BoundingBox bounds;
        unsigned char areaID;  // RecastPolyArea
    };

    struct BuildDataBase
    {
        // Geometry data
        std::vector<osg::Vec3> vertices;
        std::vector<int> indices;

        /// Offmesh connection data
        std::vector<osg::Vec3> offMeshVertices;
        std::vector<float> offMeshRadii;
        std::vector<unsigned short> offMeshFlags;
        std::vector<unsigned char> offMeshAreas;
        std::vector<unsigned char> offMeshDir;

        /// Pretransformed navigation areas
        std::vector<AreaStub> navAreas;

        // Recast members
        rcHeightfield* heightField;
        rcCompactHeightfield* compactHeightField;
        rcContext* context;

        BuildDataBase(rcContext* c)
            : heightField(NULL), compactHeightField(NULL), context(c) {}
        virtual ~BuildDataBase()
        { rcFreeHeightField(heightField); rcFreeCompactHeightfield(compactHeightField); }
    };

    struct SimpleBuildData : public BuildDataBase
    {
        rcContourSet* contourSet;
        rcPolyMesh* polyMesh;
        rcPolyMeshDetail* polyMeshDetail;

        SimpleBuildData(rcContext* c)
            : BuildDataBase(c), contourSet(NULL), polyMesh(NULL), polyMeshDetail(NULL) {}
        virtual ~SimpleBuildData()
        {
            rcFreeContourSet(contourSet); rcFreePolyMesh(polyMesh);
            rcFreePolyMeshDetail(polyMeshDetail);
        }
    };

    struct DynamicBuildData : public BuildDataBase
    {
        dtTileCacheContourSet* contourSet;
        dtTileCachePolyMesh* polyMesh;
        rcHeightfieldLayerSet* heightFieldLayers;
        dtTileCacheAlloc* alloc;

        DynamicBuildData(dtTileCacheAlloc* a, rcContext* c)
            : BuildDataBase(c), contourSet(NULL), polyMesh(NULL), heightFieldLayers(NULL), alloc(a) {}
        virtual ~DynamicBuildData()
        {
            dtFreeTileCacheContourSet(alloc, contourSet);
            dtFreeTileCachePolyMesh(alloc, polyMesh);
            rcFreeHeightfieldLayerSet(heightFieldLayers);
        }
    };

}
