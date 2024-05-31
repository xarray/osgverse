#include <osg/io_utils>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include "RecastManager.h"
#include "RecastManager_Private.h"
#include "RecastManager_Builder.h"
using namespace osgVerse;

bool RecastManager::buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                               const osg::BoundingBox& worldBounds, const osg::Vec2i& tileStart,
                               const osg::Vec2i& tileEnd)
{
    NavData* navData = static_cast<NavData*>(_recastData.get());
    const float tileEdgeLength = _settings.tileSize * _settings.cellSize;
    for (int y = tileStart[1]; y <= tileEnd[1]; ++y)
        for (int x = tileStart[0]; x <= tileEnd[0]; ++x)
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
            for (size_t i = 0; i < va.size(); ++i)
            { const osg::Vec3& v = va[i]; build.vertices.push_back(osg::Vec3(v[0], v[2], -v[1])); }

            for (size_t i = 0; i < indices.size(); i += 3)
            {
                unsigned int id0 = indices[i + 0], id1 = indices[i + 1], id2 = indices[i + 2];
                if (cfgBounds.contains(build.vertices[id0]) || cfgBounds.contains(build.vertices[id1]) ||
                    cfgBounds.contains(build.vertices[id2]))
                { build.indices.push_back(id0); build.indices.push_back(id1); build.indices.push_back(id2); }
            }

            // TODO: how to add off-mesh connections and nav-areas?
            if (build.vertices.empty() || build.indices.empty()) continue;

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
                unsigned int numTriangles = build.indices.size() / 3;
                std::vector<unsigned char> triAreas(numTriangles); memset(&triAreas[0], 0, numTriangles);
                rcMarkWalkableTriangles(build.context, cfg.walkableSlopeAngle,
                                        (float*)build.vertices.data(), build.vertices.size(),
                                        build.indices.data(), numTriangles, &triAreas[0]);

                bool ok = rcRasterizeTriangles(build.context, (float*)build.vertices.data(), build.vertices.size(),
                                               build.indices.data(), &triAreas[0], numTriangles,
                                               *build.heightField, cfg.walkableClimb);
                if (!ok) OSG_WARN << "[RecastManager] Failed to rasterize triangles" << std::endl;

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
        }
    return false;
}

osg::Node* RecastManager::getDebugMesh() const
{
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    NavData* navData = static_cast<NavData*>(_recastData.get());

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
        geom->setVertexArray(va);
        geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, va->size()));
        geode->addDrawable(geom.get());
    }
    osgUtil::SmoothingVisitor smv; geode->accept(smv);
    return geode.release();
}
