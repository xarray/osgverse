#include <algorithm>
#include <iostream>
#include <osg/io_utils>
#include <osgUtil/CullVisitor>
#include "GaussianGeometry.h"
using namespace osgVerse;

GaussianGeometry::GaussianGeometry()
:   osg::Geometry(), _degrees(0)
{
    setUseDisplayList(false);
    setUseVertexBufferObjects(true);
}

GaussianGeometry::GaussianGeometry(const GaussianGeometry& copy, const osg::CopyOp& copyop)
: osg::Geometry(copy, copyop), _degrees(copy._degrees) {}

osg::Program* GaussianGeometry::createProgram(osg::Shader* vs, osg::Shader* gs, osg::Shader* fs)
{
    osg::Program* program = new osg::Program;
    program->setParameter(GL_GEOMETRY_VERTICES_OUT_EXT, 4);
    program->setParameter(GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
    program->setParameter(GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
    program->addShader(vs); program->addShader(gs); program->addShader(fs);

    program->addBindAttribLocation("osg_Covariance0", 1);
    program->addBindAttribLocation("osg_Covariance1", 2);
    program->addBindAttribLocation("osg_Covariance2", 3);
    program->addBindAttribLocation("osg_R_SH0", 4);
    program->addBindAttribLocation("osg_G_SH0", 5);
    program->addBindAttribLocation("osg_B_SH0", 6);
    program->addBindAttribLocation("osg_R_SH1", 7);
    program->addBindAttribLocation("osg_G_SH1", 8);
    program->addBindAttribLocation("osg_B_SH1", 9);
    program->addBindAttribLocation("osg_R_SH2", 10);
    program->addBindAttribLocation("osg_G_SH2", 11);
    program->addBindAttribLocation("osg_B_SH2", 12);
    program->addBindAttribLocation("osg_R_SH3", 13);
    program->addBindAttribLocation("osg_G_SH3", 14);
    program->addBindAttribLocation("osg_B_SH3", 15);
    return program;
}

class GaussianUniformCallback : public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
        if (cv && cv->getCurrentCamera())
        {
            osg::StateSet* ss = node->getOrCreateStateSet();
            osg::Uniform* invScreen = ss->getOrCreateUniform("InvScreenResolution", osg::Uniform::FLOAT_VEC2);
            osg::Uniform* nearFar = ss->getOrCreateUniform("NearFarPlanes", osg::Uniform::FLOAT_VEC2);

            const osg::Viewport* vp = cv->getCurrentCamera()->getViewport();
            if (vp) invScreen->set(osg::Vec2(1.0f / vp->width(), 1.0f / vp->height()));

            double fov = 0.0, aspect = 0.0, znear = 0.0, zfar = 0.0;
            cv->getCurrentCamera()->getProjectionMatrixAsPerspective(fov, aspect, znear, zfar);
            nearFar->set(osg::Vec2(znear, zfar));
        }
        traverse(node, nv);
    }
};

osg::BoundingSphere GaussianGeometry::computeBound() const
{
    osg::BoundingSphere bs = osg::Geometry::computeBound();
    bs._radius *= 2.0f; return bs;  // FIXME: too simple method to compute bound
}

osg::BoundingBox GaussianGeometry::computeBoundingBox() const
{
    osg::BoundingBox bb = osg::Geometry::computeBoundingBox();
    osg::Vec3 d = bb._max - bb._min;
    bb._min -= d; bb._max += d;
    return bb;  // FIXME: too simple method to compute bound
}

osg::NodeCallback* GaussianGeometry::createUniformCallback()
{ return new GaussianUniformCallback; }

static osg::Matrix transpose(const osg::Matrix& m)
{
    return osg::Matrix(m(0, 0), m(1, 0), m(2, 0), m(3, 0), m(0, 1), m(1, 1), m(2, 1), m(3, 1),
                       m(0, 2), m(1, 2), m(2, 2), m(3, 2), m(0, 3), m(1, 3), m(2, 3), m(3, 3));
}

void GaussianGeometry::setScaleAndRotation(osg::Vec3Array* vArray, osg::QuatArray* qArray,
                                           osg::FloatArray* alphas)
{
    if (!vArray || !qArray) return; if (vArray->size() != qArray->size()) return;
    osg::Vec4Array* cov0 = new osg::Vec4Array(vArray->size());
    osg::Vec4Array* cov1 = new osg::Vec4Array(vArray->size());
    osg::Vec4Array* cov2 = new osg::Vec4Array(vArray->size());

    for (size_t i = 0; i < vArray->size(); ++i)
    {
        float a = (alphas == NULL) ? 1.0f : alphas->at(i);
        const osg::Vec3& scale = (*vArray)[i]; osg::Quat quat = (*qArray)[i];
        if (!quat.zeroRotation()) { double l = quat.length(); if (l > 0.0) quat = quat / l; }

        osg::Matrix R(quat), S = osg::Matrix::scale(scale);
        osg::Matrix cov = R * S * transpose(S) * transpose(R);
        (*cov0)[i] = osg::Vec4(cov(0, 0), cov(1, 0), cov(2, 0), a);
        (*cov1)[i] = osg::Vec4(cov(0, 1), cov(1, 1), cov(2, 1), 1.0f);
        (*cov2)[i] = osg::Vec4(cov(0, 2), cov(1, 2), cov(2, 2), 1.0f);
    }
    setVertexAttribArray(1, cov0); setVertexAttribBinding(1, BIND_PER_VERTEX);
    setVertexAttribArray(2, cov1); setVertexAttribBinding(2, BIND_PER_VERTEX);
    setVertexAttribArray(3, cov2); setVertexAttribBinding(3, BIND_PER_VERTEX);
}

///////////////////////// GaussianSorter /////////////////////////

void GaussianSorter::addGeometry(GaussianGeometry* geom)
{ _geometries.insert(geom); }

void GaussianSorter::removeGeometry(GaussianGeometry* geom)
{
    std::set<osg::ref_ptr<GaussianGeometry>>::iterator it = _geometries.find(geom);
    if (it != _geometries.end()) _geometries.erase(it);
}

void GaussianSorter::cull(const osg::Matrix& view)
{
    std::vector<osg::ref_ptr<GaussianGeometry>> _geometriesToSort;
    for (std::set<osg::ref_ptr<GaussianGeometry>>::iterator it = _geometries.begin();
         it != _geometries.end();)
    {
        GaussianGeometry* gs = (*it).get();  // remove those only ref-ed by the sorter
        if (!gs || (gs && gs->referenceCount() < 2)) { it = _geometries.erase(it); continue; }

        osg::MatrixList matrices = gs->getWorldMatrices();
        if (matrices.empty()) { it = _geometries.erase(it); continue; }
        cull(gs, matrices[0], view); ++it;
    }
}

void GaussianSorter::cull(GaussianGeometry* geom, const osg::Matrix& model, const osg::Matrix& view)
{
    osg::Vec3Array* pos = geom->getPosition(); if (!pos) return;
    osg::DrawElementsUInt* indices = (geom->getNumPrimitiveSets() > 0)
                                   ? static_cast<osg::DrawElementsUInt*>(geom->getPrimitiveSet(0)) : NULL;
    if (!indices)
    {
        indices = new osg::DrawElementsUInt(GL_POINTS);
        indices->resize(pos->size()); geom->addPrimitiveSet(indices);
        for (unsigned int i = 0; i < pos->size(); ++i) (*indices)[i] = i;
    }
    else indices->dirty();

    osg::Matrix localToEye = model * view;
    switch (_method)
    {
    case CPU_SORT:
        std::sort(indices->begin(), indices->end(), [&pos, &localToEye](unsigned int lhs, unsigned int rhs)
            {
                const osg::Vec3 &v0 = (*pos)[lhs] * localToEye, &v1 = (*pos)[rhs] * localToEye;
                return v0.z() < v1.z();  // sort primitive indices
            });
        break;
    default: break;
    }
}
