#include <osg/TriangleIndexFunctor>
#include <osg/Geometry>
#include <osg/Geode>
#include <iostream>
#include <ApproxMVBB/ComputeApproxMVBB.hpp>
#include <VHACD/VHACD.h>
#include "Utilities.h"
using namespace osgVerse;

struct CollectVertexOperator
{
    void operator()(unsigned int i1, unsigned int i2, unsigned int i3)
    {
        if (vertices)
        {
            vertices->push_back((*inputData)[i1] * matrix);
            vertices->push_back((*inputData)[i2] * matrix);
            vertices->push_back((*inputData)[i3] * matrix);
        }

        if (indices)
        {
            indices->push_back(baseIndex + i1); indices->push_back(baseIndex + i2);
            indices->push_back(baseIndex + i3);
        }
    }

    CollectVertexOperator() : inputData(NULL), vertices(NULL), indices(NULL) {}
    osg::Vec3Array* inputData;
    std::vector<osg::Vec3>* vertices;
    std::vector<unsigned int>* indices;
    osg::Matrix matrix; unsigned int baseIndex;
};

BoundingVolumeVisitor::BoundingVolumeVisitor()
: osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ACTIVE_CHILDREN) {}

void BoundingVolumeVisitor::reset()
{
    _matrixStack.clear();
    _vertices.clear(); _indices.clear();
}

void BoundingVolumeVisitor::apply(osg::Transform& node)
{
    osg::Matrix matrix;
    if (!_matrixStack.empty()) matrix = _matrixStack.back();
    node.computeLocalToWorldMatrix(matrix, this);

    pushMatrix(matrix);
    traverse(node);
    popMatrix();
}

void BoundingVolumeVisitor::apply(osg::Geode& node)
{
    osg::Matrix matrix;
    if (_matrixStack.size() > 0) matrix = _matrixStack.back();

    for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
    {
        osg::Geometry* geom = node.getDrawable(i)->asGeometry();
        if (!geom) continue;

        osg::TriangleIndexFunctor<CollectVertexOperator> functor;
        functor.inputData = static_cast<osg::Vec3Array*>(geom->getVertexArray());
        functor.vertices = &_vertices; functor.indices = &_indices;
        functor.matrix = matrix; functor.baseIndex = _vertices.size();
        geom->accept(functor);
    }
    traverse(node);
}

osg::BoundingBox BoundingVolumeVisitor::computeOBB(osg::Quat& rotation, float relativeExtent, int numSamples)
{
    ApproxMVBB::Matrix3Dyn points(3, _vertices.size());
    for (size_t i = 0; i < _vertices.size(); ++i)
    {
        const osg::Vec3& v = _vertices[i];
        points.col(i) << v[0], v[1], v[2];
    }

    ApproxMVBB::OOBB oobb = ApproxMVBB::approximateMVBB(points, 0.001, numSamples);
    oobb.expandToMinExtentRelative(relativeExtent);
    rotation.set(oobb.m_q_KI.x(), oobb.m_q_KI.y(), oobb.m_q_KI.z(), oobb.m_q_KI.w());
    return osg::BoundingBox(osg::Vec3(oobb.m_minPoint.x(), oobb.m_minPoint.y(), oobb.m_minPoint.z()),
                            osg::Vec3(oobb.m_maxPoint.x(), oobb.m_maxPoint.y(), oobb.m_maxPoint.z()));
}

bool BoundingVolumeVisitor::computeKDop(std::vector<ConvexHull>& hulls, int maxConvexHulls)
{
    VHACD::IVHACD::Parameters param;
    param.m_maxConvexHulls = maxConvexHulls;
    if (_vertices.empty() || _indices.empty()) return false;

    VHACD::IVHACD* iface = VHACD::CreateVHACD();
    if (!iface->Compute((float*)&_vertices[0], _vertices.size(),
                        &_indices[0], _indices.size() / 3, param))
    { return false; }

    uint32_t numHulls = iface->GetNConvexHulls();
    hulls.resize(numHulls);
    for (uint32_t i = 0; i < numHulls; ++i)
    {
        VHACD::IVHACD::ConvexHull ch;
        iface->GetConvexHull(i, ch);
        for (uint32_t p = 0; p < ch.m_nPoints; ++p)
        {
            osg::Vec3 pt(ch.m_points[3 * p], ch.m_points[3 * p + 1],
                         ch.m_points[3 * p + 2]);
            hulls[i].points.push_back(pt);
        }

        for (uint32_t t = 0; t < ch.m_nTriangles; ++t)
        {
            hulls[i].triangles.push_back(ch.m_triangles[3 * t]);
            hulls[i].triangles.push_back(ch.m_triangles[3 * t + 1]);
            hulls[i].triangles.push_back(ch.m_triangles[3 * t + 2]);
        }
    }
    iface->Release();
    return true;
}
