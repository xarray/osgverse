#include "SkyBox.h"
#include "Utilities.h"
#include <osg/io_utils>
#include <osg/Depth>
#include <osg/Drawable>
#include <osg/Matrix>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/TexMat>
#include <osg/TexEnv>
#include <osg/TexGen>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
#include "Pipeline.h"
using namespace osgVerse;

#if 0
struct TexMatCallback : public osg::NodeCallback
{
public:
    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        if (cv)
        {
            const osg::Matrix& MV = *(cv->getModelViewMatrix());
            osg::Quat q = MV.getRotate();
            const osg::Matrix C = osg::Matrix::rotate(q.inverse());

            osg::Matrix moveMx = osg::Matrix::translate(osg::Vec3());;
            float intervalTime = nv->getFrameStamp()->getSimulationTime() - _startTime;
            if (intervalTime > 0.01f)
            {
                _startTime = nv->getFrameStamp()->getSimulationTime();
                const osg::Matrix R = osg::Matrix::rotate(osg::DegreesToRadians(260.0f), 0.0f, 0.0f, 1.0f)
                                    * osg::Matrix::rotate(osg::DegreesToRadians(-90.0f), 1.0f, 0.0f, 0.0f);
                _texMat.setMatrix(C * R);
            }
        }
        traverse(node, nv);
    }

    TexMatCallback(osg::TexMat& tm) : _texMat(tm), _startTime(0.0f) {}
    osg::TexMat& _texMat; float _startTime;
};
#endif

/* SkyBox */

SkyBox::SkyBox(Pipeline* p)
    : osg::Transform(), _pipeline(p)
{
}

SkyBox::SkyBox(const SkyBox& copy, const osg::CopyOp& copyop)
    : osg::Transform(copy, copyop), _skymap(copy._skymap)
{
}

osg::Camera* SkyBox::createSkyCamera()
{
    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setName("SkyCamera");
    postCamera->setClearMask(0);
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    postCamera->setProjectionMatrixAsPerspective(30.0f, 1.0f, 1.0f, 1000.0f);
    postCamera->setReferenceFrame(osg::Camera::ABSOLUTE_RF);
    return postCamera.release();
}

void SkyBox::setEnvironmentMap(const std::string& path, const std::string& ext, bool rightHanded)
{
    std::string fileName = "";
    osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;

    fileName = path + "posx." + ext;  // LEFT
    osg::ref_ptr<osg::Image> imagePosX = osgDB::readImageFile(fileName);
    fileName = path + "negx." + ext;  // RIGHT
    osg::ref_ptr<osg::Image> imageNegX = osgDB::readImageFile(fileName);

    fileName = path + "posz." + ext;  // FRONT
    osg::ref_ptr<osg::Image> imagePosZ = osgDB::readImageFile(fileName);
    fileName = path + "negz." + ext;  // BACK
    osg::ref_ptr<osg::Image> imageNegZ = osgDB::readImageFile(fileName);

    fileName = path + (rightHanded ? "posy." : "negy.") + ext;  // DOWN
    osg::ref_ptr<osg::Image> imagePosY = osgDB::readImageFile(fileName);
    fileName = path + (rightHanded ? "negy." : "posy.") + ext;  // UP
    osg::ref_ptr<osg::Image> imageNegY = osgDB::readImageFile(fileName);

    if (imagePosX.get() && imageNegX.get() && imagePosY.get() && imageNegY.get() &&
        imagePosZ.get() && imageNegZ.get()/*&& imageCenter.get()*/)
    {
        cubemap->setImage(osg::TextureCubeMap::POSITIVE_X, imagePosX.get());
        cubemap->setImage(osg::TextureCubeMap::NEGATIVE_X, imageNegX.get());
        cubemap->setImage(osg::TextureCubeMap::POSITIVE_Y, imagePosY.get());
        cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Y, imageNegY.get());
        cubemap->setImage(osg::TextureCubeMap::POSITIVE_Z, imagePosZ.get());
        cubemap->setImage(osg::TextureCubeMap::NEGATIVE_Z, imageNegZ.get());

        cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);

        cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
        cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    }
    setEnvironmentMap(cubemap.get(), true);
}

void SkyBox::setEnvironmentMap(osg::Image* image)
{
    if (image)
    {
        osg::ref_ptr<osg::Texture2D> skymap = new osg::Texture2D;
        skymap->setImage(image); skymap->setResizeNonPowerOfTwoHint(false);
        skymap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        skymap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        skymap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        skymap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        setEnvironmentMap(skymap.get(), false);
    }
}

void SkyBox::setEnvironmentMap(osg::Texture* tex, bool asCubemap)
{
    _skymap = tex;
    initialize(asCubemap, osg::Matrixf::rotate(-osg::PI_2, osg::X_AXIS));
}

void SkyBox::setSkyColor(const osg::Vec4ub& color)
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA8);
    
    osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
    (*ptr) = color; setEnvironmentMap(image.get());
}

void SkyBox::initialize(bool asCube, const osg::Matrixf& texMat)
{
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet;
    unsigned int values = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE;
#if 0
    osg::ref_ptr<osg::TexGen> tg = new osg::TexGen;
    tg->setMode(osg::TexGen::REFLECTION_MAP);
    stateset->setTextureAttributeAndModes(0, tg.get(), values);

    osg::TexMat* tm = new osg::TexMat;
    stateset->setTextureAttribute(0, tm);
    setCullCallback(new TexMatCallback(*tm));
#endif

    stateset->setTextureAttributeAndModes(0, _skymap.get(), values);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setFunction(osg::Depth::LEQUAL);
    depth->setRange(1.0, 1.0);
    stateset->setAttributeAndModes(depth, values);
    stateset->setRenderBinDetails(-9999, "RenderBin");

    osg::Shader* vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl");
    osg::Shader* fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl");
    vs->setName("SkyBox_SHADER_VS"); fs->setName("SkyBox_SHADER_FS");
    if (_pipeline.valid())
    {
        std::vector<std::string> defs;
        if (asCube) defs.push_back("#define VERSE_CUBEMAP_SKYBOX 1");
        Pipeline::createShaderDefinitions(vs, _pipeline->getTargetVersion(),
                                          _pipeline->getGlslTargetVersion());
        Pipeline::createShaderDefinitions(fs, _pipeline->getTargetVersion(),
                                          _pipeline->getGlslTargetVersion(), defs);
    }

    osg::Program* program = new osg::Program;
    program->setName("SkyBox_PROGRAM");
    program->addShader(vs); program->addShader(fs);
    stateset->setAttributeAndModes(program);
    stateset->addUniform(new osg::Uniform("SkyTexture", (int)0));
    stateset->addUniform(new osg::Uniform("SkyTextureMatrix", texMat));

    osg::ref_ptr<osg::Drawable> drawable = new osg::ShapeDrawable(
        new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 1.0f));
    drawable->setComputeBoundingBoxCallback(new DisableBoundingBoxCallback);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->setCullingActive(false);
    geode->setStateSet(stateset.get());
    geode->addDrawable(drawable.get());
    geode->setName("SkyBoxGeode");
    addChild(geode.get());
}

bool SkyBox::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
#if COMPLEX_SKYBOX
        const osg::RefMatrix* modelView = cv->getModelViewMatrix();
        const osg::RefMatrix* proj = cv->getProjectionMatrix();

        osg::Vec3d eye = osg::Matrix::inverse(*modelView).getTrans();
        double far = (*proj)(3, 2) / (1.0 + (*proj)(2, 2));
        matrix.preMult(osg::Matrix::scale(far, far, far) * osg::Matrix::translate(eye));
#else
        const osg::RefMatrix* proj = cv->getProjectionMatrix();
        double far = (*proj)(3, 2) / (1.0 + (*proj)(2, 2));
        matrix.preMult(osg::Matrix::scale(far, far, far) * osg::Matrix::translate(cv->getEyeLocal()));
#endif
        return true;
    }
    else
        return osg::Transform::computeLocalToWorldMatrix(matrix, nv);
}

bool SkyBox::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = static_cast<osgUtil::CullVisitor*>(nv);
#if COMPLEX_SKYBOX
        const osg::RefMatrix* modelView = cv->getModelViewMatrix();
        const osg::RefMatrix* proj = cv->getProjectionMatrix();

        osg::Vec3d eye = osg::Matrix::inverse(*modelView).getTrans();
        double invFar = (1.0 + (*proj)(2, 2)) / (*proj)(3, 2);
        matrix.postMult(osg::Matrix::translate(-eye) * osg::Matrix::scale(invFar, invFar, invFar));
#else
        const osg::RefMatrix* proj = cv->getProjectionMatrix();
        double invFar = (1.0 + (*proj)(2, 2)) / (*proj)(3, 2);
        matrix.postMult(osg::Matrix::translate(-cv->getEyeLocal()) *
                        osg::Matrix::scale(invFar, invFar, invFar));
#endif
        return true;
    }
    else
        return osg::Transform::computeWorldToLocalMatrix(matrix, nv);
}
