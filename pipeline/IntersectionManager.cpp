#include "IntersectionManager.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
using namespace osgVerse;

class LineSegmentIntersectorEx : public osgUtil::LineSegmentIntersector
{
public:
    LineSegmentIntersectorEx(const osg::Vec3d& s, const osg::Vec3d& e)
        : osgUtil::LineSegmentIntersector(s, e) {}

    LineSegmentIntersectorEx(CoordinateFrame cf, const osg::Vec3d& s, const osg::Vec3d& e)
        : osgUtil::LineSegmentIntersector(cf, s, e) {}

    LineSegmentIntersectorEx(CoordinateFrame cf, double x, double y)
        : osgUtil::LineSegmentIntersector(cf, x, y) {}

    virtual Intersector* clone(osgUtil::IntersectionVisitor& iv)
    {
        if (_coordinateFrame == MODEL && iv.getModelMatrix() == 0)
        {
            osg::ref_ptr<LineSegmentIntersectorEx> lsi = new LineSegmentIntersectorEx(_start, _end);
            lsi->_parent = this;
            lsi->_nodesToIgnore = _nodesToIgnore;
            lsi->_intersectionLimit = this->_intersectionLimit;
            return lsi.release();
        }

        // compute the matrix that takes this Intersector from its CoordinateFrame into the local MODEL coordinate frame
        // that geometry in the scene graph will always be in.
        osg::Matrix matrix;
        switch (_coordinateFrame)
        {
        case WINDOW:
            if (iv.getWindowMatrix()) matrix.preMult(*iv.getWindowMatrix());
            if (iv.getProjectionMatrix()) matrix.preMult(*iv.getProjectionMatrix());
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case PROJECTION:
            if (iv.getProjectionMatrix()) matrix.preMult(*iv.getProjectionMatrix());
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case VIEW:
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case MODEL:
            if (iv.getModelMatrix()) matrix = *iv.getModelMatrix();
            break;
        }

        osg::Matrix inverse;
        inverse.invert(matrix);

        osg::ref_ptr<LineSegmentIntersectorEx> lsi = new LineSegmentIntersectorEx(_start * inverse, _end * inverse);
        lsi->_parent = this;
        lsi->_nodesToIgnore = _nodesToIgnore;
        lsi->_intersectionLimit = this->_intersectionLimit;
        return lsi.release();
    }

    virtual bool enter(const osg::Node& node)
    {
        if (_nodesToIgnore.find(const_cast<osg::Node*>(&node)) != _nodesToIgnore.end()) return false;
        return osgUtil::LineSegmentIntersector::enter(node);
    }

    std::set<osg::Node*> _nodesToIgnore;
};

class PolytopeIntersectorEx : public osgUtil::PolytopeIntersector
{
public:
    PolytopeIntersectorEx(const osg::Polytope& polytope)
        : osgUtil::PolytopeIntersector(polytope) {}

    PolytopeIntersectorEx(CoordinateFrame cf, const osg::Polytope& polytope)
        : osgUtil::PolytopeIntersector(cf, polytope) {}

    PolytopeIntersectorEx(CoordinateFrame cf, double xMin, double yMin, double xMax, double yMax)
        : osgUtil::PolytopeIntersector(cf, xMin, yMin, xMax, yMax) {}

    Intersector* clone(osgUtil::IntersectionVisitor& iv)
    {
        if (_coordinateFrame == MODEL && iv.getModelMatrix() == 0)
        {
            osg::ref_ptr<PolytopeIntersectorEx> pi = new PolytopeIntersectorEx(_polytope);
            pi->_parent = this;
            pi->_nodesToIgnore = _nodesToIgnore;
            pi->_intersectionLimit = this->_intersectionLimit;
            pi->_referencePlane = this->_referencePlane;
            return pi.release();
        }

        // compute the matrix that takes this Intersector from its CoordinateFrame into the local MODEL coordinate frame
        // that geometry in the scene graph will always be in.
        osg::Matrix matrix;
        switch (_coordinateFrame)
        {
        case WINDOW:
            if (iv.getWindowMatrix()) matrix.preMult(*iv.getWindowMatrix());
            if (iv.getProjectionMatrix()) matrix.preMult(*iv.getProjectionMatrix());
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case PROJECTION:
            if (iv.getProjectionMatrix()) matrix.preMult(*iv.getProjectionMatrix());
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case VIEW:
            if (iv.getViewMatrix()) matrix.preMult(*iv.getViewMatrix());
            if (iv.getModelMatrix()) matrix.preMult(*iv.getModelMatrix());
            break;
        case MODEL:
            if (iv.getModelMatrix()) matrix = *iv.getModelMatrix();
            break;
        }

        osg::Polytope transformedPolytope;
        transformedPolytope.setAndTransformProvidingInverse(_polytope, matrix);

        osg::ref_ptr<PolytopeIntersectorEx> pi = new PolytopeIntersectorEx(transformedPolytope);
        pi->_parent = this;
        pi->_nodesToIgnore = _nodesToIgnore;
        pi->_intersectionLimit = this->_intersectionLimit;
        pi->_referencePlane = this->_referencePlane;
        pi->_referencePlane.transformProvidingInverse(matrix);
        return pi.release();
    }

    virtual bool enter(const osg::Node& node)
    {
        if (_nodesToIgnore.find(const_cast<osg::Node*>(&node)) != _nodesToIgnore.end()) return false;
        return osgUtil::PolytopeIntersector::enter(node);
    }

    std::set<osg::Node*> _nodesToIgnore;
};

static void applyLinesegmentIntersectionCondition(
    osgUtil::IntersectionVisitor& iv,
    LineSegmentIntersectorEx* intersector,
    IntersectionCondition* condition)
{
    // TODO: infinityMask
    intersector->_nodesToIgnore = condition->nodesToIgnore;
    intersector->setCoordinateFrame(condition->coordinateFrame);
    intersector->setIntersectionLimit(condition->limit);
    iv.setReadCallback(condition->readCallback.get());
    iv.setTraversalMask(condition->traversalMask);
}

static void applyPolytopeIntersectionCondition(
    osgUtil::IntersectionVisitor& iv,
    PolytopeIntersectorEx* intersector,
    IntersectionCondition* condition)
{
    intersector->_nodesToIgnore = condition->nodesToIgnore;
    intersector->setCoordinateFrame(condition->coordinateFrame);
    intersector->setIntersectionLimit(condition->limit);
    iv.setReadCallback(condition->readCallback.get());
    iv.setTraversalMask(condition->traversalMask);
}

static void saveLinesegmentIntersectionResult(
    const osgUtil::LineSegmentIntersector::Intersection& intersection,
    IntersectionResult& result)
{
    result.nodePath = intersection.nodePath;
    result.drawable = intersection.drawable;
    result.matrix = intersection.matrix.valid() ? *(intersection.matrix) : osg::Matrix();
    result.distanceToReference = intersection.ratio;
    result.primitiveIndex = intersection.primitiveIndex;
    result.intersectPoints.push_back(intersection.localIntersectionPoint);

    IntersectionResult::IntersectTextureData tdata;
    tdata.first = intersection.getTextureLookUp(tdata.second);
    result.intersectTextureData.push_back(tdata);
}

static void savePolytopeIntersectionResult(
    const osgUtil::PolytopeIntersector::Intersection& intersection,
    IntersectionResult& result)
{
    result.nodePath = intersection.nodePath;
    result.drawable = intersection.drawable;
    result.matrix = intersection.matrix.valid() ? *(intersection.matrix) : osg::Matrix();
    for (unsigned int i = 0; i < intersection.numIntersectionPoints; ++i)
        result.intersectPoints.push_back(intersection.intersectionPoints[i]);
    result.distanceToReference = intersection.distance;
    result.primitiveIndex = intersection.primitiveIndex;
}

namespace osgVerse
{
    IntersectionResult findNearestIntersection(
        osg::Node* node, double xNorm, double yNorm, IntersectionCondition* condition)
    {
        osg::ref_ptr<LineSegmentIntersectorEx> intersector =
            new LineSegmentIntersectorEx(osgUtil::Intersector::PROJECTION, xNorm, yNorm);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
        {
            applyLinesegmentIntersectionCondition(iv, intersector.get(), condition);
            intersector->setCoordinateFrame(osgUtil::Intersector::PROJECTION);
        }
        intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);
        node->accept(iv);

        IntersectionResult result;
        if (intersector->containsIntersections())
            saveLinesegmentIntersectionResult(intersector->getFirstIntersection(), result);
        return result;
    }

    IntersectionResult findNearestIntersection(
        osg::Node* node, const osg::Vec3d& s, const osg::Vec3d& e, IntersectionCondition* condition)
    {
        osg::ref_ptr<LineSegmentIntersectorEx> intersector =
            new LineSegmentIntersectorEx(osgUtil::Intersector::MODEL, s, e);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
            applyLinesegmentIntersectionCondition(iv, intersector.get(), condition);
        intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);
        node->accept(iv);

        IntersectionResult result;
        if (intersector->containsIntersections())
            saveLinesegmentIntersectionResult(intersector->getFirstIntersection(), result);
        return result;
    }

    std::vector<IntersectionResult> findAllIntersections(
        osg::Node* node, const osg::Vec3d& s, const osg::Vec3d& e, IntersectionCondition* condition)
    {
        osg::ref_ptr<LineSegmentIntersectorEx> intersector =
            new LineSegmentIntersectorEx(osgUtil::Intersector::MODEL, s, e);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
            applyLinesegmentIntersectionCondition(iv, intersector.get(), condition);
        node->accept(iv);

        std::vector<IntersectionResult> results;
        if (intersector->containsIntersections())
        {
            osgUtil::LineSegmentIntersector::Intersections& all = intersector->getIntersections();
            for (osgUtil::LineSegmentIntersector::Intersections::const_iterator itr = all.begin();
                itr != all.end(); ++itr)
            {
                IntersectionResult result;
                saveLinesegmentIntersectionResult(*itr, result);
                results.push_back(result);
            }
        }
        return results;
    }

    IntersectionResult findNearestIntersection(
        osg::Node* node, double xmin, double ymin, double xmax, double ymax,
        IntersectionCondition* condition)
    {
        osg::ref_ptr<PolytopeIntersectorEx> intersector =
            new PolytopeIntersectorEx(osgUtil::Intersector::PROJECTION, xmin, ymin, xmax, ymax);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
        {
            applyPolytopeIntersectionCondition(iv, intersector.get(), condition);
            intersector->setCoordinateFrame(osgUtil::Intersector::PROJECTION);
        }
        intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);
        node->accept(iv);

        IntersectionResult result;
        if (intersector->containsIntersections())
            savePolytopeIntersectionResult(intersector->getFirstIntersection(), result);
        return result;
    }

    IntersectionResult findNearestIntersection(
        osg::Node* node, const osg::Polytope& polytope, IntersectionCondition* condition)
    {
        osg::ref_ptr<PolytopeIntersectorEx> intersector =
            new PolytopeIntersectorEx(osgUtil::Intersector::MODEL, polytope);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
            applyPolytopeIntersectionCondition(iv, intersector.get(), condition);
        intersector->setIntersectionLimit(osgUtil::Intersector::LIMIT_NEAREST);
        node->accept(iv);

        IntersectionResult result;
        if (intersector->containsIntersections())
            savePolytopeIntersectionResult(intersector->getFirstIntersection(), result);
        return result;
    }

    std::vector<IntersectionResult> findAllIntersections(
        osg::Node* node, const osg::Polytope& polytope, IntersectionCondition* condition)
    {
        osg::ref_ptr<PolytopeIntersectorEx> intersector =
            new PolytopeIntersectorEx(osgUtil::Intersector::MODEL, polytope);

        osgUtil::IntersectionVisitor iv(intersector.get());
        if (condition)
            applyPolytopeIntersectionCondition(iv, intersector.get(), condition);
        node->accept(iv);

        std::vector<IntersectionResult> results;
        if (intersector->containsIntersections())
        {
            osgUtil::PolytopeIntersector::Intersections& all = intersector->getIntersections();
            for (osgUtil::PolytopeIntersector::Intersections::const_iterator itr = all.begin();
                itr != all.end(); ++itr)
            {
                IntersectionResult result;
                savePolytopeIntersectionResult(*itr, result);
                results.push_back(result);
            }
        }
        return results;
    }
}
