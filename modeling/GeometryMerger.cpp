#include "GeometryMerger.h"
#include "Utilities.h"
#include <osg/Texture2D>
using namespace osgVerse;

GeometryMerger::GeometryMerger()
{}

GeometryMerger::~GeometryMerger()
{}

osg::Geometry* GeometryMerger::process(const std::vector<osg::Geometry*>& geomList)
{
    // Collect textures and make atlas

    // Recompute texture coords

    // Concatenate arrays and primitive-sets
    return NULL;
}
