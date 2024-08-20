#ifndef MANA_MODELING_GEOMERY_MAPPER_HPP
#define MANA_MODELING_GEOMERY_MAPPER_HPP

#include <math.h>
#include <map>
#include <vector>
#include <iostream>
#include <osg/Geometry>

namespace osgVerse
{
    /** Map vertex attributes and statesets from one node to another */
    class GeometryMapper
    {
    public:
        GeometryMapper();
        ~GeometryMapper();

        /** Map attributes of closest vertex */
        void mapAttributes(osg::Node* source, osg::Node* target);

        /** Compute similarity of two nodes */
        float computeSimilarity(osg::Node* source, osg::Node* target);

    protected:
    };
}

#endif
