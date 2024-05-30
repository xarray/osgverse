#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <recastnavigation/Recast/Recast.h>
#include <recastnavigation/Detour/DetourNavMesh.h>
#include <recastnavigation/Detour/DetourNavMeshBuilder.h>
#include <recastnavigation/DetourTileCache/DetourTileCache.h>
#include <recastnavigation/DetourTileCache/DetourTileCacheBuilder.h>
#include <recastnavigation/DetourCrowd/DetourCrowd.h>
#include <modeling/Utilities.h>
#include "RecastManager.h"
#include <chrono>
using namespace osgVerse;

class BuildContext : public rcContext
{
public:
    BuildContext() : rcContext() {}

protected:
    virtual void doResetLog() {}
    virtual void doResetTimers() { _timers.clear(); }

    virtual void doLog(const rcLogCategory category, const char* msg, const int len)
    {
        switch (category)
        {
        case RC_LOG_WARNING:
            OSG_WARN << "[BuildContext] Warning: " << std::string(msg, len) << std::endl;
        case RC_LOG_ERROR:
            OSG_FATAL << "[BuildContext] Error: " << std::string(msg, len) << std::endl;
        default:
            OSG_INFO << "[BuildContext] " << std::string(msg, len) << std::endl;
        }
    }

    virtual void doStartTimer(const rcTimerLabel label)
    { _timers[label].first = std::chrono::steady_clock::now(); }

    virtual void doStopTimer(const rcTimerLabel label)
    { _timers[label].second = std::chrono::steady_clock::now(); }

    virtual int doGetAccumulatedTime(const rcTimerLabel label) const
    {
        std::map<rcTimerLabel, TimePair>::const_iterator itr = _timers.find(label);
        if (itr == _timers.end()) return -1;
        return std::chrono::duration_cast<std::chrono::seconds>(itr->second.second - itr->second.first).count();
    }

    typedef std::pair<std::chrono::steady_clock::time_point, std::chrono::steady_clock::time_point> TimePair;
    std::map<rcTimerLabel, TimePair> _timers;
};

class NavData : public osg::Referenced
{
public:
    NavData() : navMesh(NULL), navQuery(NULL), crowd(NULL)
    { context = new BuildContext; }

    static int calculateMaxTiles(const osg::BoundingBoxd& bb, osg::Vec2i& begin, osg::Vec2i& end,
                                 int tileSize, float cellSize)
    {
        if (!bb.valid()) return 0;
        const float tileEdgeLength = tileSize * cellSize;
        const osg::Vec2 beginTileIndex(bb.xMin() / tileEdgeLength, bb.yMin() / tileEdgeLength);
        const osg::Vec2 endTileIndex(bb.xMax() / tileEdgeLength, bb.yMax() / tileEdgeLength);
        const osg::Vec2 tileRange = endTileIndex - beginTileIndex + osg::Vec2();

        int numTiles = (int)tileRange.x() * (int)tileRange.y();
        begin = osg::Vec2i((int)beginTileIndex.x(), (int)beginTileIndex.y());
        end = osg::Vec2i((int)endTileIndex.x(), (int)endTileIndex.y());
        return osg::Image::computeNearestPowerOfTwo(numTiles);
    }

    static unsigned int logBaseTwo(unsigned value)
    {
        // http://graphics.stanford.edu/~seander/bithacks.html#IntegerLogObvious
        unsigned ret = 0; while (value >>= 1) ++ret; return ret;
    }

    void clear()
    {
        if (navMesh != NULL) dtFreeNavMesh(navMesh); navMesh = NULL;
        if (navQuery != NULL) dtFreeNavMeshQuery(navQuery); navQuery = NULL;
    }

    dtNavMesh* navMesh;
    dtNavMeshQuery* navQuery;
    dtCrowd* crowd;
    BuildContext* context;

protected:
    virtual ~NavData()
    { clear(); delete context; }
};

//////////// RecastManager ///////////////

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

    NavData* navData = static_cast<NavData*>(_recastData.get());
    osg::ComputeBoundsVisitor cbv; node->accept(cbv);
    int maxTiles = navData->calculateMaxTiles(
        cbv.getBoundingBox(), tStart, tEnd, _settings.tileSize, _settings.cellSize);

    dtNavMeshParams params{};
    params.tileWidth = _settings.tileSize * _settings.cellSize;
    params.tileHeight = params.tileWidth;
    params.maxPolys = 1u << (22 - navData->logBaseTwo(maxTiles));
    params.maxTiles = maxTiles;
    navData->clear(); navData->navMesh = dtAllocNavMesh();

    if (dtStatusFailed(navData->navMesh->init(&params))) return false;
    return buildTiles(collector.getVertices(), collector.getTriangles(), tStart, tEnd, navData->context);
}
