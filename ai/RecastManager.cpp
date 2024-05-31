#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include "RecastManager.h"
#include "RecastManager_Private.h"
using namespace osgVerse;

RecastManager::RecastManager()
{
    _recastData = new NavData;
}

RecastManager::~RecastManager()
{
}

bool RecastManager::build(osg::Node* node)
{
    MeshCollector collector; osg::Vec2i tStart, tEnd;
    collector.setWeldingVertices(true);
    collector.setUseGlobalVertices(true);
    if (!node) return false; else node->accept(collector);
    if (collector.getVertices().empty()) return false;

    osg::ComputeBoundsVisitor cbv; node->accept(cbv);
    osg::BoundingBox worldBounds = cbv.getBoundingBox();
    osg::Vec3 padding(_settings.padding * _settings.tileSize,
                      _settings.padding * _settings.tileSize, _settings.padding);
    worldBounds._min -= padding; worldBounds._max += padding;

    NavData* navData = static_cast<NavData*>(_recastData.get());
    int maxTiles = navData->calculateMaxTiles(
        worldBounds, tStart, tEnd, _settings.tileSize, _settings.cellSize);
    dtNavMeshParams params{};
    params.tileWidth = _settings.tileSize * _settings.cellSize;
    params.tileHeight = params.tileWidth;
    params.maxPolys = 1u << (22 - navData->logBaseTwo(maxTiles));
    params.maxTiles = maxTiles;
    navData->clear(); navData->navMesh = dtAllocNavMesh();

    if (dtStatusFailed(navData->navMesh->init(&params))) return false;
    return buildTiles(collector.getVertices(), collector.getTriangles(), worldBounds, tStart, tEnd);
}
