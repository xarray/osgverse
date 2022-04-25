#ifndef MANA_MODELING_UTILITIES_HPP
#define MANA_MODELING_UTILITIES_HPP

#include <osg/Transform>
#include <osg/Geometry>
#include <osg/Camera>

namespace osgVerse
{
    
    struct ConvexHull
    {
        std::vector<osg::Vec3> points;
        std::vector<unsigned int> triangles;
    };

    class BoundingVolumeVisitor : public osg::NodeVisitor
    {
    public:
        BoundingVolumeVisitor();
        inline void pushMatrix(osg::Matrix& matrix) { _matrixStack.push_back(matrix); }
        inline void popMatrix() { _matrixStack.pop_back(); }

        virtual void reset();
        virtual void apply(osg::Transform& transform);
        virtual void apply(osg::Geode& node);

        virtual void apply(osg::Node& node) { traverse(node); }
        virtual void apply(osg::Drawable& node) {}  // do nothing
        virtual void apply(osg::Geometry& geometry) {}  // do nothing

        /** Returned value is in OBB coordinates, using rotation to convert it */
        osg::BoundingBox computeOBB(osg::Quat& rotation, float relativeExtent = 0.1f, int numSamples = 500);

        /** Get a list of convex hulls to contain this node */
        bool computeKDop(std::vector<ConvexHull>& hulls, int maxConvexHulls = 24);

    protected:
        typedef std::vector<osg::Matrix> MatrixStack;
        MatrixStack _matrixStack;
        std::vector<osg::Vec3> _vertices;
        std::vector<unsigned int> _indices;
    };

}

#endif
