#include "IntersectionManager.h"
#include <algorithm>
#include <functional>
#include <iostream>
#include <osg/io_utils>
#include <osg/Texture>
#include <osg/TexMat>
#include <osg/TriangleIndexFunctor>
using namespace osgVerse;

static osg::Texture* getTextureLookUp(const osgUtil::LineSegmentIntersector::Intersection& it, osg::Vec3& tc)
{
    osg::Geometry* geometry = it.drawable.valid() ? it.drawable->asGeometry() : 0;
    osg::Vec3Array* vertices = geometry ? dynamic_cast<osg::Vec3Array*>(geometry->getVertexArray()) : 0;

    if (vertices)
    {
        if (it.indexList.size() == 3 && it.ratioList.size() == 3)
        {
            unsigned int i1 = it.indexList[0];
            unsigned int i2 = it.indexList[1];
            unsigned int i3 = it.indexList[2];
            float r1 = it.ratioList[0];
            float r2 = it.ratioList[1];
            float r3 = it.ratioList[2];

            osg::Array* texcoords = (geometry->getNumTexCoordArrays() > 0) ? geometry->getTexCoordArray(0) : 0;
            osg::FloatArray* texcoords_FloatArray = dynamic_cast<osg::FloatArray*>(texcoords);
            osg::Vec2Array* texcoords_Vec2Array = dynamic_cast<osg::Vec2Array*>(texcoords);
            osg::Vec3Array* texcoords_Vec3Array = dynamic_cast<osg::Vec3Array*>(texcoords);
            if (texcoords_FloatArray)
            {
                // we have tex coord array so now we can compute the final tex coord at the point of intersection.
                float tc1 = (*texcoords_FloatArray)[i1];
                float tc2 = (*texcoords_FloatArray)[i2];
                float tc3 = (*texcoords_FloatArray)[i3];
                tc.x() = tc1 * r1 + tc2 * r2 + tc3 * r3;
            }
            else if (texcoords_Vec2Array)
            {
                // we have tex coord array so now we can compute the final tex coord at the point of intersection.
                const osg::Vec2& tc1 = (*texcoords_Vec2Array)[i1];
                const osg::Vec2& tc2 = (*texcoords_Vec2Array)[i2];
                const osg::Vec2& tc3 = (*texcoords_Vec2Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
            }
            else if (texcoords_Vec3Array)
            {
                // we have tex coord array so now we can compute the final tex coord at the point of intersection.
                const osg::Vec3& tc1 = (*texcoords_Vec3Array)[i1];
                const osg::Vec3& tc2 = (*texcoords_Vec3Array)[i2];
                const osg::Vec3& tc3 = (*texcoords_Vec3Array)[i3];
                tc.x() = tc1.x()*r1 + tc2.x()*r2 + tc3.x()*r3;
                tc.y() = tc1.y()*r1 + tc2.y()*r2 + tc3.y()*r3;
                tc.z() = tc1.z()*r1 + tc2.z()*r2 + tc3.z()*r3;
            }
            else
                return 0;
        }

        const osg::TexMat* activeTexMat = 0;
        const osg::Texture* activeTexture = 0;
        if (it.drawable->getStateSet())
        {
            const osg::TexMat* texMat = dynamic_cast<osg::TexMat*>(
                it.drawable->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXMAT));
            if (texMat) activeTexMat = texMat;

            const osg::Texture* texture = dynamic_cast<osg::Texture*>(
                it.drawable->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            if (texture) activeTexture = texture;
        }

        for (osg::NodePath::const_reverse_iterator itr = it.nodePath.rbegin();
            itr != it.nodePath.rend() && (!activeTexMat || !activeTexture); ++itr)
        {
            const osg::Node* node = *itr;
            if (node->getStateSet())
            {
                if (!activeTexMat)
                {
                    const osg::TexMat* texMat = dynamic_cast<const osg::TexMat*>(
                        node->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXMAT));
                    if (texMat) activeTexMat = texMat;
                }

                if (!activeTexture)
                {
                    const osg::Texture* texture = dynamic_cast<const osg::Texture*>(
                        node->getStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
                    if (texture) activeTexture = texture;
                }
            }
        }

        if (activeTexMat)
        {
            osg::Vec4 tc_transformed = osg::Vec4(tc.x(), tc.y(), tc.z(), 0.0f) * activeTexMat->getMatrix();
            tc.x() = tc_transformed.x();
            tc.y() = tc_transformed.y();
            tc.z() = tc_transformed.z();
            if (activeTexture && activeTexMat->getScaleByTextureRectangleSize())
            {
                tc.x() *= static_cast<float>(activeTexture->getTextureWidth());
                tc.y() *= static_cast<float>(activeTexture->getTextureHeight());
                tc.z() *= static_cast<float>(activeTexture->getTextureDepth());
            }
        }
        return const_cast<osg::Texture*>(activeTexture);
    }
    return 0;
}

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
    result.intersectNormals.push_back(intersection.localIntersectionNormal);
    result.ratioList.push_back(intersection.ratio);

    IntersectionResult::IntersectTextureData tdata;
    tdata.first = getTextureLookUp(intersection, tdata.second);
    result.intersectTextureData.push_back(tdata);
}

static void savePolytopeIntersectionResult(
    const osg::Polytope& polytope, const osgUtil::PolytopeIntersector::Intersection& intersection,
    IntersectionResult& result)
{
    const osg::Polytope::PlaneList& pList = polytope.getPlaneList();
    result.nodePath = intersection.nodePath;
    result.drawable = intersection.drawable;
    result.matrix = intersection.matrix.valid() ? *(intersection.matrix) : osg::Matrix();
    for (unsigned int i = 0; i < intersection.numIntersectionPoints; ++i)
    {
        osg::Vec3d pt = intersection.intersectionPoints[i];
        result.intersectPoints.push_back(pt);

        double ratio = FLT_MAX;
        for (size_t p = 0; p < pList.size(); ++p)
        { double d = pList[p].distance(pt); if (d < ratio) ratio = d; }
        result.ratioList.push_back(ratio);
    }
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
        if (intersector->containsIntersections()) savePolytopeIntersectionResult(
            intersector->getPolytope(), intersector->getFirstIntersection(), result);
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
            savePolytopeIntersectionResult(polytope, intersector->getFirstIntersection(), result);
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
                savePolytopeIntersectionResult(polytope , *itr, result);
                results.push_back(result);
            }
        }
        return results;
    }
}
