#include <osg/Version>
#include <osg/io_utils>
#include <osg/ImageUtils>
#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/MeshOptimizers>
#include <osgUtil/Tessellator>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <iostream>

#include "MeshTopology.h"
#include "Utilities.h"
using namespace osgVerse;

namespace osgVerse
{

    osg::Geometry* createLatheGeometry(const std::vector<osg::Vec3> ctrlPoints, const osg::Vec3& axis,
                                       bool withBSplinePoints, bool withCaps)
    {
        // TODO
        return NULL;
    }

}
