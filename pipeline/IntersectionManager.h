#ifndef MANA_PP_INTERSECTIONMANAGER_HPP
#define MANA_PP_INTERSECTIONMANAGER_HPP

#include <osg/Geometry>
#include <osgUtil/LineSegmentIntersector>
#include <osgUtil/PolytopeIntersector>

namespace osgVerse
{
    /** The intersection condition for defining a more detailed intersection test */
    struct IntersectionCondition
    {
        std::set<osg::Node*> nodesToIgnore;
        osg::ref_ptr<osgUtil::IntersectionVisitor::ReadCallback> readCallback;
        osgUtil::Intersector::CoordinateFrame coordinateFrame;
        osgUtil::Intersector::IntersectionLimit limit;
        unsigned int infinityMask;   // Line only: Infinite start = 1, Infinite end = 2
        unsigned int traversalMask;

        IntersectionCondition()
            : coordinateFrame(osgUtil::Intersector::MODEL), limit(osgUtil::Intersector::NO_LIMIT),
            infinityMask(0), traversalMask(0xffffffff) {}
    };

    /** The intersection result structure */
    struct IntersectionResult
    {
        osg::NodePath nodePath;
        osg::ref_ptr<osg::Drawable> drawable;
        osg::Matrix matrix;
        std::vector<osg::Vec3d> intersectPoints;
        double distanceToReference;
        unsigned int primitiveIndex;

        typedef std::pair<osg::Texture*, osg::Vec3> IntersectTextureData;
        std::vector<IntersectTextureData> intersectTextureData;  // only available for line intersections

        IntersectionResult() : distanceToReference(FLT_MAX), primitiveIndex(0) {}
        osg::Vec3d getWorldIntersectPoint(unsigned int i = 0) const { return intersectPoints[i] * matrix; }
    };

    /** Find nearest intersection result with projected coordinates to form a linesegment */
    extern IntersectionResult findNearestIntersection(
        osg::Node* node, double xNorm, double yNorm, IntersectionCondition* condition = 0);

    /** Find nearest intersection result with a 3D linesegment */
    extern IntersectionResult findNearestIntersection(
        osg::Node* node, const osg::Vec3d&, const osg::Vec3d&, IntersectionCondition* condition = 0);

    /** Find all intersection results with a 3D linesegment */
    extern std::vector<IntersectionResult> findAllIntersections(
        osg::Node* node, const osg::Vec3d&, const osg::Vec3d&, IntersectionCondition* condition = 0);

    /** Find nearest intersection result with projected coordinates to form a polytope */
    extern IntersectionResult findNearestIntersection(
        osg::Node* node, double xmin, double ymin, double xmax, double ymax,
        IntersectionCondition* condition = 0);

    /** Find nearest intersection result with a 3D polytope */
    extern IntersectionResult findNearestIntersection(
        osg::Node* node, const osg::Polytope& polytope, IntersectionCondition* condition = 0);

    /** Find all intersection results with a 3D polytope */
    extern std::vector<IntersectionResult> findAllIntersections(
        osg::Node* node, const osg::Polytope& polytope, IntersectionCondition* condition = 0);
}

#endif
