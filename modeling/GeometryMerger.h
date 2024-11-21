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
        typedef std::pair<osg::Geometry*, osg::Matrix> GeometryPair;
        GeometryMerger();
        ~GeometryMerger();

        osg::Geometry* process(const std::vector<GeometryPair>& geomList, size_t offset,
                               size_t size = 0, int maxTextureSize = 4096);
        osg::Node* processAsOctree(const std::vector<GeometryPair>& geomList);

    protected:
    };
}

#endif
