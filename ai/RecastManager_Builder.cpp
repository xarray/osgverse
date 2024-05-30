#include <osg/io_utils>
#include "RecastManager.h"
#include "RecastManager_Builder.h"
#include <chrono>
using namespace osgVerse;

bool RecastManager::buildTiles(const std::vector<osg::Vec3>& va, const std::vector<unsigned int>& indices,
                               const osg::Vec2i& tileStart, const osg::Vec2i& tileEnd, void* context)
{
    return false;
}
