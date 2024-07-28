#include <osg/io_utils>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include "RecastManager.h"
#include "RecastManager_Private.h"
#include "RecastManager_Builder.h"
using namespace osgVerse;

namespace
{
    struct BoundsItem
    {
        float bmin[2];
        float bmax[2];
        int i;
    };

    static int compareItemX(const void* va, const void* vb)
    {
        const BoundsItem* a = (const BoundsItem*)va;
        const BoundsItem* b = (const BoundsItem*)vb;
        if (a->bmin[0] < b->bmin[0]) return -1;
        if (a->bmin[0] > b->bmin[0]) return 1;
        return 0;
    }

    static int compareItemY(const void* va, const void* vb)
    {
        const BoundsItem* a = (const BoundsItem*)va;
        const BoundsItem* b = (const BoundsItem*)vb;
        if (a->bmin[1] < b->bmin[1]) return -1;
        if (a->bmin[1] > b->bmin[1]) return 1;
        return 0;
    }

    static void calcExtends(const BoundsItem* items, const int /*nitems*/,
                            const int imin, const int imax, float* bmin, float* bmax)
    {
        bmin[0] = items[imin].bmin[0];
        bmin[1] = items[imin].bmin[1];
        bmax[0] = items[imin].bmax[0];
        bmax[1] = items[imin].bmax[1];
        for (int i = imin + 1; i < imax; ++i)
        {
            const BoundsItem& it = items[i];
            if (it.bmin[0] < bmin[0]) bmin[0] = it.bmin[0];
            if (it.bmin[1] < bmin[1]) bmin[1] = it.bmin[1];
            if (it.bmax[0] > bmax[0]) bmax[0] = it.bmax[0];
            if (it.bmax[1] > bmax[1]) bmax[1] = it.bmax[1];
        }
    }

    inline int longestAxis(float x, float y)
    { return y > x ? 1 : 0; }

    static void subdivide(BoundsItem* items, int nitems, int imin, int imax, int trisPerChunk,
                          int& curNode, rcChunkyTriMeshNode* nodes, const int maxNodes,
                          int& curTri, int* outTris, const int* inTris)
    {
        int inum = imax - imin, icur = curNode;
        if (curNode >= maxNodes) return;
        rcChunkyTriMeshNode& node = nodes[curNode++];

        if (inum <= trisPerChunk)
        {
            calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);  // Leaf
            node.i = curTri; node.n = inum;  // Copy triangles
            for (int i = imin; i < imax; ++i)
            {
                const int* src = &inTris[items[i].i * 3];
                int* dst = &outTris[curTri * 3]; curTri++;
                dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
            }
        }
        else
        {
            calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);  // Split
            int	axis = longestAxis(node.bmax[0] - node.bmin[0], node.bmax[1] - node.bmin[1]);
            if (axis == 0)  // Sort along x-axis
                qsort(items + imin, static_cast<size_t>(inum), sizeof(BoundsItem), compareItemX);
            else if (axis == 1)  // Sort along y-axis
                qsort(items + imin, static_cast<size_t>(inum), sizeof(BoundsItem), compareItemY);

            int isplit = imin + inum / 2;
            subdivide(items, nitems, imin, isplit, trisPerChunk, curNode, nodes, maxNodes, curTri, outTris, inTris);
            subdivide(items, nitems, isplit, imax, trisPerChunk, curNode, nodes, maxNodes, curTri, outTris, inTris);
            int iescape = curNode - icur; node.i = -iescape;  // Negative index means escape.
        }
    }


    static bool checkOverlapSegment(const float p[2], const float q[2],
                                    const float bmin[2], const float bmax[2])
    {
        static const float EPSILON = 1e-6f;
        float tmin = 0, tmax = 1; float d[2];
        d[0] = q[0] - p[0]; d[1] = q[1] - p[1];
        for (int i = 0; i < 2; i++)
        {
            if (fabsf(d[i]) < EPSILON)
            {   // Ray is parallel to slab. No hit if origin not within slab
                if (p[i] < bmin[i] || p[i] > bmax[i]) return false;
            }
            else
            {   // Compute intersection t value of ray with near and far plane of slab
                float ood = 1.0f / d[i];
                float t1 = (bmin[i] - p[i]) * ood;
                float t2 = (bmax[i] - p[i]) * ood;
                if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                if (t1 > tmin) tmin = t1; if (t2 < tmax) tmax = t2;
                if (tmin > tmax) return false;
            }
        }
        return true;
    }

    static bool checkOverlapRect(const float amin[2], const float amax[2],
                                 const float bmin[2], const float bmax[2])
    {
        bool overlap = true;
        overlap = (amin[0] > bmax[0] || amax[0] < bmin[0]) ? false : overlap;
        overlap = (amin[1] > bmax[1] || amax[1] < bmin[1]) ? false : overlap;
        return overlap;
    }
}

bool rcChunkyTriMesh::createChunkyTriMesh(const float* verts, const int* tris, int ntris,
                                          int trisPerChunk, rcChunkyTriMesh* cm)
{
    int nchunks = (ntris + trisPerChunk - 1) / trisPerChunk;
    cm->nodes = new rcChunkyTriMeshNode[nchunks * 4];
    if (!cm->nodes) return false; cm->tris = new int[ntris * 3];
    if (!cm->tris) return false; cm->ntris = ntris;

    // Build tree
    BoundsItem* items = new BoundsItem[ntris];
    if (!items) return false;
    for (int i = 0; i < ntris; i++)
    {
        const int* t = &tris[i * 3];
        BoundsItem& it = items[i]; it.i = i;

        // Calc triangle XZ bounds.
        it.bmin[0] = it.bmax[0] = verts[t[0] * 3 + 0];
        it.bmin[1] = it.bmax[1] = verts[t[0] * 3 + 2];
        for (int j = 1; j < 3; ++j)
        {
            const float* v = &verts[t[j] * 3];
            if (v[0] < it.bmin[0]) it.bmin[0] = v[0];
            if (v[2] < it.bmin[1]) it.bmin[1] = v[2];
            if (v[0] > it.bmax[0]) it.bmax[0] = v[0];
            if (v[2] > it.bmax[1]) it.bmax[1] = v[2];
        }
    }

    int curTri = 0, curNode = 0;
    subdivide(items, ntris, 0, ntris, trisPerChunk, curNode, cm->nodes, nchunks * 4, curTri, cm->tris, tris);
    delete[] items; cm->nnodes = curNode;

    // Calc max tris per node.
    cm->maxTrisPerChunk = 0;
    for (int i = 0; i < cm->nnodes; ++i)
    {
        rcChunkyTriMeshNode& node = cm->nodes[i];
        const bool isLeaf = node.i >= 0; if (!isLeaf) continue;
        if (node.n > cm->maxTrisPerChunk) cm->maxTrisPerChunk = node.n;
    }
    return true;
}

std::vector<int> rcChunkyTriMesh::getChunksOverlappingRect(const rcChunkyTriMesh* cm, float bmin[2], float bmax[2])
{
    int i = 0, n = 0; std::vector<int> idList;
    while (i < cm->nnodes)
    {
        const rcChunkyTriMeshNode* node = &cm->nodes[i];
        const bool overlap = checkOverlapRect(bmin, bmax, node->bmin, node->bmax);
        const bool isLeafNode = node->i >= 0;
        if (isLeafNode && overlap) { idList.push_back(i); n++; }
        if (overlap || isLeafNode) i++;
        else { const int escapeIndex = -node->i; i += escapeIndex; }
    }
    return idList;
}

std::vector<int> rcChunkyTriMesh::getChunksOverlappingSegment(const rcChunkyTriMesh* cm, float p[2], float q[2])
{
    int i = 0, n = 0; std::vector<int> idList;
    while (i < cm->nnodes)
    {
        const rcChunkyTriMeshNode* node = &cm->nodes[i];
        const bool overlap = checkOverlapSegment(p, q, node->bmin, node->bmax);
        const bool isLeafNode = node->i >= 0;
        if (isLeafNode && overlap) { idList.push_back(i); n++; }
        if (overlap || isLeafNode) i++;
        else { const int escapeIndex = -node->i; i += escapeIndex; }
    }
    return idList;
}

bool RecastManager::buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                               const osg::BoundingBoxd& worldBounds, const osg::Vec2d& tileStart,
                               const osg::Vec2d& tileEnd)
{
    std::vector<osg::Vec3> va1(va.size()); if (va.empty() || indices.empty()) return false;
    for (size_t i = 0; i < va.size(); ++i) { const osg::Vec3& v = va[i]; va1[i] = osg::Vec3(v[0], v[2], -v[1]); }

    rcChunkyTriMesh* chunkyMesh = new rcChunkyTriMesh; std::vector<int> chunkyIdList;
    if (!rcChunkyTriMesh::createChunkyTriMesh((float*)&va1[0], (int*)&indices[0],
                                              indices.size() / 3, 256, chunkyMesh))
    {
        OSG_WARN << "[RecastManager] Failed to build chunky tri-mesh" << std::endl;
        delete chunkyMesh; chunkyMesh = NULL;
    }

    NavData* navData = static_cast<NavData*>(_recastData.get());
    const float tileEdgeLength = _settings.tileSize * _settings.cellSize;
    for (int y = (int)tileStart[1]; y <= (int)tileEnd[1]; ++y)
        for (int x = (int)tileStart[0]; x <= (int)tileEnd[0]; ++x)
        {
            rcConfig cfg; memset(&cfg, 0, sizeof(cfg));
            cfg.cs = _settings.cellSize; cfg.ch = _settings.cellHeight;
            cfg.walkableSlopeAngle = _settings.agentMaxSlope;
            cfg.walkableHeight = (int)floor(0.5f + _settings.agentHeight / cfg.ch);
            cfg.walkableClimb = (int)floor(_settings.agentMaxClimb / cfg.ch);
            cfg.walkableRadius = (int)floor(0.5f + _settings.agentRadius / cfg.cs);
            cfg.maxEdgeLen = (int)(_settings.edgeMaxLen / cfg.cs);
            cfg.maxSimplificationError = _settings.edgeMaxError;
            cfg.minRegionArea = (int)sqrtf(_settings.regionMinSize);
            cfg.mergeRegionArea = (int)sqrtf(_settings.regionMergeSize);
            cfg.maxVertsPerPoly = _settings.vertsPerPoly; cfg.tileSize = _settings.tileSize;
            cfg.borderSize = cfg.walkableRadius + 3; // Add padding
            cfg.width = cfg.tileSize + cfg.borderSize * 2;
            cfg.height = cfg.tileSize + cfg.borderSize * 2;
            cfg.detailSampleDist = (_settings.detailSampleDist < 0.9f)
                                 ? 0.0f : (cfg.cs * _settings.detailSampleDist);
            cfg.detailSampleMaxError = cfg.ch * _settings.detailSampleMaxError;

            const osg::Vec3 minBB(x * tileEdgeLength, worldBounds.zMin(), y * tileEdgeLength);
            const osg::Vec3 maxBB((x + 1) * tileEdgeLength, worldBounds.zMax(), (y + 1) * tileEdgeLength);
            rcVcopy(cfg.bmin, minBB.ptr()); rcVcopy(cfg.bmax, maxBB.ptr());
            cfg.bmin[0] -= cfg.borderSize * cfg.cs; cfg.bmax[0] += cfg.borderSize * cfg.cs;
            cfg.bmin[1] -= _settings.padding; cfg.bmax[1] += _settings.padding;
            cfg.bmin[2] -= cfg.borderSize * cfg.cs; cfg.bmax[2] += cfg.borderSize * cfg.cs;
            
            // Fill build data
            SimpleBuildData build(navData->context); osg::BoundingBox cfgBounds(minBB, maxBB);
            navData->navMesh->removeTile(navData->navMesh->getTileRefAt(x, y, 0), NULL, NULL);

            // TODO: how to add off-mesh connections and nav-areas?
            if (chunkyMesh == NULL)
            {
                build.vertices.assign(va1.begin(), va1.end());
                for (size_t i = 0; i < indices.size(); i += 3)
                {
                    unsigned int id0 = indices[i + 0], id1 = indices[i + 1], id2 = indices[i + 2];
                    if (cfgBounds.contains(build.vertices[id0]) || cfgBounds.contains(build.vertices[id1]) ||
                        cfgBounds.contains(build.vertices[id2]))
                    { build.indices.push_back(id0); build.indices.push_back(id1); build.indices.push_back(id2); }
                }
                if (build.vertices.empty() || build.indices.empty()) continue;
            }
            else
            {
                float tbmin[2]; tbmin[0] = cfg.bmin[0]; tbmin[1] = cfg.bmin[2];
                float tbmax[2]; tbmax[0] = cfg.bmax[0]; tbmax[1] = cfg.bmax[2];
                chunkyIdList = rcChunkyTriMesh::getChunksOverlappingRect(chunkyMesh, tbmin, tbmax);
                if (chunkyIdList.empty()) continue;
            }

            // Create and config height-field
            build.heightField = rcAllocHeightfield();
            if (!rcCreateHeightfield(build.context, *build.heightField,
                                     cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
            {
                OSG_WARN << "[RecastManager] Failed to build height-field of tile: "
                         << x << ", " << y << std::endl; continue;
            }
            else
            {
                if (chunkyMesh == NULL)
                {
                    unsigned int numTriangles = build.indices.size() / 3;
                    std::vector<unsigned char> triAreas(numTriangles); memset(&triAreas[0], 0, numTriangles);
                    rcMarkWalkableTriangles(build.context, cfg.walkableSlopeAngle,
                                            (float*)build.vertices.data(), build.vertices.size(),
                                            build.indices.data(), numTriangles, &triAreas[0]);

                    // TODO: mark non-walkable?
                    bool ok = rcRasterizeTriangles(
                            build.context, (float*)build.vertices.data(), build.vertices.size(),
                            build.indices.data(), &triAreas[0], numTriangles,
                            *build.heightField, cfg.walkableClimb);
                    if (!ok) OSG_WARN << "[RecastManager] Failed to rasterize triangles" << std::endl;
                }
                else
                {
                    std::vector<unsigned char> triAreas(chunkyMesh->maxTrisPerChunk);
                    for (int i = 0; i < chunkyIdList.size(); ++i)
                    {
                        const rcChunkyTriMeshNode& node = chunkyMesh->nodes[chunkyIdList[i]];
                        const int* ptrT = &chunkyMesh->tris[node.i * 3]; const int numT = node.n;
                        memset(&triAreas[0], 0, numT * sizeof(unsigned char));
                        rcMarkWalkableTriangles(build.context, cfg.walkableSlopeAngle,
                                                (float*)va1.data(), va1.size(), ptrT, numT, &triAreas[0]);

                        // TODO: mark non-walkable?
                        bool ok = rcRasterizeTriangles(
                            build.context, (float*)va1.data(), va1.size(), ptrT,
                            &triAreas[0], numT, *build.heightField, cfg.walkableClimb);
                        if (!ok) OSG_WARN << "[RecastManager] Failed to rasterize triangles" << std::endl;
                    }
                }

                rcFilterLowHangingWalkableObstacles(build.context, cfg.walkableClimb, *build.heightField);
                rcFilterWalkableLowHeightSpans(build.context, cfg.walkableHeight, *build.heightField);
                rcFilterLedgeSpans(build.context, cfg.walkableHeight, cfg.walkableClimb, *build.heightField);
            }

            // Create and config compact height-field
            build.compactHeightField = rcAllocCompactHeightfield();
            if (!rcBuildCompactHeightfield(build.context, cfg.walkableHeight, cfg.walkableClimb,
                                           *build.heightField, *build.compactHeightField))
            {
                OSG_WARN << "[RecastManager] Failed to build compact height-field of tile: "
                         << x << ", " << y << std::endl; continue;
            }
            else
            {
                if (!rcErodeWalkableArea(build.context, cfg.walkableRadius, *build.compactHeightField))
                {
                    OSG_WARN << "[RecastManager] Failed to erode compact height-field of tile: "
                             << x << ", " << y << std::endl; continue;
                }
            }

            // Mark area volumes
            for (unsigned i = 0; i < build.navAreas.size(); ++i)
            {
                rcMarkBoxArea(build.context,
                    build.navAreas[i].bounds._min.ptr(), build.navAreas[i].bounds._max.ptr(),
                    build.navAreas[i].areaID, *build.compactHeightField);
            }

            // Build regions
            if (_settings.partitionType == PARTITION_WATERSHED)
            {
                if (!rcBuildDistanceField(build.context, *build.compactHeightField))
                {
                    OSG_WARN << "[RecastManager] Failed to build distance fields of tile: "
                             << x << ", " << y << std::endl; continue;
                }
                if (!rcBuildRegions(build.context, *build.compactHeightField,
                                    cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
                {
                    OSG_WARN << "[RecastManager] Failed to build regions of tile: "
                             << x << ", " << y << std::endl; continue;
                }
            }
            else if (_settings.partitionType == PARTITION_MONOTONE)
            {
                if (!rcBuildRegionsMonotone(build.context, *build.compactHeightField,
                                            cfg.borderSize, cfg.minRegionArea, cfg.mergeRegionArea))
                {
                    OSG_WARN << "[RecastManager] Failed to build monotone regions of tile: "
                             << x << ", " << y << std::endl; continue;
                }
            }
            else
            {
                OSG_WARN << "[RecastManager] Unknown partition type of tile: "
                         << x << ", " << y << std::endl; continue;
            }

            // Build contour set
            build.contourSet = rcAllocContourSet();
            if (!rcBuildContours(build.context, *build.compactHeightField, cfg.maxSimplificationError,
                                 cfg.maxEdgeLen, *build.contourSet))
            {
                OSG_WARN << "[RecastManager] Failed to create contours of tile: "
                         << x << ", " << y << std::endl; continue;
            }

            // Build poly-mesh and details
            build.polyMesh = rcAllocPolyMesh();
            if (!rcBuildPolyMesh(build.context, *build.contourSet, cfg.maxVertsPerPoly, *build.polyMesh))
            {
                OSG_WARN << "[RecastManager] Failed to triangulate contours of tile: "
                         << x << ", " << y << std::endl; continue;
            }

            build.polyMeshDetail = rcAllocPolyMeshDetail();
            if (!rcBuildPolyMeshDetail(build.context, *build.polyMesh, *build.compactHeightField,
                                       cfg.detailSampleDist, cfg.detailSampleMaxError, *build.polyMeshDetail))
            {
                OSG_WARN << "[RecastManager] Failed to build detailed poly mesh of tile: "
                         << x << ", " << y << std::endl; continue;
            }

            // Set polygon flags
            for (int i = 0; i < build.polyMesh->npolys; ++i)
            {
                unsigned char area = build.polyMesh->areas[i];
                if (area == POLYAREA_WATER) build.polyMesh->flags[i] = POLYFLAGS_SWIM;
                else if (area != POLYAREA_NULL) build.polyMesh->flags[i] = POLYFLAGS_WALK;
                // TODO: custom area/flags
            }

            // Create nav-mesh data
            dtNavMeshCreateParams params; memset(&params, 0, sizeof(params));
            params.verts = build.polyMesh->verts; params.vertCount = build.polyMesh->nverts;
            params.polys = build.polyMesh->polys; params.polyCount = build.polyMesh->npolys;
            params.polyAreas = build.polyMesh->areas; params.polyFlags = build.polyMesh->flags;
            params.nvp = build.polyMesh->nvp; params.detailMeshes = build.polyMeshDetail->meshes;
            params.detailVerts = build.polyMeshDetail->verts;
            params.detailVertsCount = build.polyMeshDetail->nverts;
            params.detailTris = build.polyMeshDetail->tris;
            params.detailTriCount = build.polyMeshDetail->ntris;
            params.walkableHeight = _settings.agentHeight;
            params.walkableRadius = _settings.agentRadius;
            params.walkableClimb = _settings.agentMaxClimb;
            params.tileX = x; params.tileY = y;
            rcVcopy(params.bmin, build.polyMesh->bmin);
            rcVcopy(params.bmax, build.polyMesh->bmax);
            params.cs = cfg.cs; params.ch = cfg.ch;
            params.buildBvTree = true;
            if (!build.offMeshRadii.empty())
            {
                // Add off-mesh connections if have them
                params.offMeshConCount = build.offMeshRadii.size();
                params.offMeshConVerts = (float*)build.offMeshVertices.data();
                params.offMeshConRad = &build.offMeshRadii[0];
                params.offMeshConFlags = &build.offMeshFlags[0];
                params.offMeshConAreas = &build.offMeshAreas[0];
                params.offMeshConDir = &build.offMeshDir[0];
            }

            unsigned char* resultData = NULL;
            int resultDataSize = 0;
            if (!dtCreateNavMeshData(&params, &resultData, &resultDataSize))
            {
                OSG_WARN << "[RecastManager] Failed to build navigation mesh of tile: "
                         << x << ", " << y << std::endl; continue;
            }
            else if (dtStatusFailed(navData->navMesh->addTile(resultData, resultDataSize,
                                                              DT_TILE_FREE_DATA, 0, NULL)))
            {
                OSG_WARN << "[RecastManager] Failed to add tile to recast manager: "
                         << x << ", " << y << std::endl; dtFree(resultData); continue;
            }
            //OSG_NOTICE << "[RecastManager] Tile added: " << x << ", " << y << " (End = "
            //           << tileEnd[0] << ", " << tileEnd[1] << ")" << std::endl;
        }
    delete chunkyMesh;
    return initializeQuery();
}

bool RecastManager::read(std::istream& in)
{
    float orig[3], tileW = 0.0f, tileH = 0.0f;
    osg::Vec3 o; int maxPolys = 0, maxTiles = 0;
    in.read((char*)orig, sizeof(float) * 3); o.set(orig[0], orig[1], orig[2]);
    in.read((char*)&tileW, sizeof(float)); in.read((char*)&tileH, sizeof(float));
    in.read((char*)&maxTiles, sizeof(int)); in.read((char*)&maxPolys, sizeof(int));
    if (!initializeNavMesh(o, tileW, tileH, maxPolys, maxTiles)) return false;

    NavData* navData = static_cast<NavData*>(_recastData.get());
    while (!in.eof())
    {
        int x = 0, y = 0, dataSize = 0;
        in.read((char*)&x, sizeof(int)); in.read((char*)&y, sizeof(int));
        in.read((char*)&dataSize, sizeof(int));

        unsigned char* tData = (unsigned char*)dtAlloc(dataSize, DT_ALLOC_PERM);
        in.read((char*)tData, dataSize);
        if (dtStatusFailed(navData->navMesh->addTile(tData, dataSize, DT_TILE_FREE_DATA, 0, NULL)))
        {
            OSG_WARN << "[RecastManager] Failed to add tile to recast manager: "
                     << x << ", " << y << std::endl; dtFree(tData); continue;
        }
    }
    return initializeQuery();
}

bool RecastManager::save(std::ostream& out)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navMesh) return false;

    const dtNavMesh* navMesh = navData->navMesh;
    const dtNavMeshParams* params = navMesh->getParams();
    out.write((char*)params->orig, sizeof(float) * 3);
    out.write((char*)&(params->tileWidth), sizeof(float));
    out.write((char*)&(params->tileHeight), sizeof(float));
    out.write((char*)&(params->maxTiles), sizeof(int));
    out.write((char*)&(params->maxPolys), sizeof(int));

    for (int t = 0; t < navMesh->getMaxTiles(); ++t)
    {
        const dtMeshTile* tile = navMesh->getTile(t);
        if (!tile->header || !tile->dataSize) continue;
        out.write((char*)&(tile->header->x), sizeof(int));
        out.write((char*)&(tile->header->y), sizeof(int));
        out.write((char*)&(tile->dataSize), sizeof(int));
        out.write((char*)tile->data, tile->dataSize);
    }
    return true;
}

osg::Node* RecastManager::getDebugMesh() const
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    NavData* navData = static_cast<NavData*>(_recastData.get());
    if (!navData->navMesh) return NULL;

    const dtNavMesh* navMesh = navData->navMesh;
    for (int t = 0; t < navMesh->getMaxTiles(); ++t)
    {
        const dtMeshTile* tile = navMesh->getTile(t);
        if (!tile->header) continue;

        osg::Vec3Array* va = new osg::Vec3Array;
        osg::Vec4Array* ca = new osg::Vec4Array;
        for (int i = 0; i < tile->header->polyCount; ++i)
        {
            dtPoly* poly = tile->polys + i;
            for (unsigned j = 0; j < poly->vertCount; ++j)
            {
                float* p0 = &(tile->verts[poly->verts[j] * 3]);
                float* p1 = &(tile->verts[poly->verts[(j + 1) % poly->vertCount] * 3]);
                va->push_back(osg::Vec3(*p0, -*(p0 + 2), *(p0 + 1)));
                va->push_back(osg::Vec3(*p1, -*(p1 + 2), *(p1 + 1)));
                ca->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 0.5f));
                ca->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 0.5f));
            }
        }

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
        geom->setName("RecastDebugMesh"); geom->setVertexArray(va);
        geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, va->size()));
        geode->addDrawable(geom.get());
    }
    osgUtil::SmoothingVisitor smv; geode->accept(smv);
    return geode.release();
}
