#ifndef MANA_MODELING_GEOMERY_MERGER_HPP
#define MANA_MODELING_GEOMERY_MERGER_HPP

#include <math.h>
#include <map>
#include <vector>
#include <iostream>
#include <osg/Geometry>

namespace osgVerse
{
    class GeometryMerger
    {
    public:
        GeometryMerger();
        ~GeometryMerger();

        osg::Geometry* process(const std::vector<osg::Geometry*>& geomList, size_t offset,
                               size_t size = 0, int maxTextureSize = 4096);

    protected:
    };
}

#endif
