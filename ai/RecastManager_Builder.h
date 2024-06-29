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

    struct rcChunkyTriMeshNode
    {
        float bmin[2];
        float bmax[2];
        int i, n;
    };

    struct rcChunkyTriMesh
    {
        inline rcChunkyTriMesh() : nodes(0), nnodes(0), ntris(0), maxTrisPerChunk(0), tris(0) {}
        inline ~rcChunkyTriMesh() { delete[] nodes; delete[] tris; }

        rcChunkyTriMeshNode* nodes;
        int nnodes, ntris, maxTrisPerChunk;
        int* tris;

        static bool createChunkyTriMesh(const float* verts, const int* tris, int ntris,
                                        int trisPerChunk, rcChunkyTriMesh* cm);
        static std::vector<int> getChunksOverlappingRect(const rcChunkyTriMesh* cm, float bmin[2], float bmax[2]);
        static std::vector<int> getChunksOverlappingSegment(const rcChunkyTriMesh* cm, float p[2], float q[2]);

    private:
        rcChunkyTriMesh(const rcChunkyTriMesh&);
        rcChunkyTriMesh& operator=(const rcChunkyTriMesh&);
    };

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
