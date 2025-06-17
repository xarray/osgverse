#include <algorithm>
#include "Eigen/Dense"
#include "Eigen/Geometry"
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

void GaussianGeometry::setPositionAndAlpha(osg::Vec3Array* v, osg::FloatArray* a)
{
    if (!v || !a) return; if (v->size() != a->size()) return;
    std::vector<osg::Vec4> result; result.reserve(v->size());
    std::transform(v->begin(), v->end(), a->begin(), std::back_inserter(result),
        [](const osg::Vec3& vN, float aN) { return osg::Vec4(vN[0], vN[1], vN[2], aN); });
    setVertexArray(new osg::Vec4Array(result.begin(), result.end()));
}

void GaussianGeometry::setScaleAndRotation(osg::Vec3Array* vArray, osg::QuatArray* qArray)
{
    if (!vArray || !qArray) return; if (vArray->size() != qArray->size()) return;
    osg::Vec3Array* cov0 = new osg::Vec3Array(vArray->size());
    osg::Vec3Array* cov1 = new osg::Vec3Array(vArray->size());
    osg::Vec3Array* cov2 = new osg::Vec3Array(vArray->size());

    for (size_t i = 0; i < vArray->size(); ++i)
    {
        const osg::Vec3& scale = (*vArray)[i];
        const osg::Quat& quat = (*qArray)[i];
        Eigen::Quaternionf q(quat[3], quat[0], quat[1], quat[2]);
        Eigen::Matrix3f R = q.normalized().toRotationMatrix();
        Eigen::Matrix3f S; S.diagonal() << scale[0], scale[1], scale[2];
        Eigen::Matrix3f cov = R * S * S.transpose() * R.transpose();

        (*cov0)[i] = osg::Vec3(cov(0, 0), cov(1, 0), cov(2, 0));
        (*cov1)[i] = osg::Vec3(cov(0, 1), cov(1, 1), cov(2, 1));
        (*cov2)[i] = osg::Vec3(cov(0, 2), cov(1, 2), cov(2, 2));
    }
    setVertexAttribArray(1, cov0); setVertexAttribBinding(1, BIND_PER_VERTEX);
    setVertexAttribArray(2, cov1); setVertexAttribBinding(2, BIND_PER_VERTEX);
    setVertexAttribArray(3, cov2); setVertexAttribBinding(3, BIND_PER_VERTEX);
}
