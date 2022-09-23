#include "SkyBox.h"
#include <osg/Depth>
#include <osg/Drawable>
#include <osg/Matrix>
#include <osg/ShapeDrawable>
#include <osg/StateSet>
#include <osg/TexEnv>
#include <osg/TexGen>
#include <osg/Texture2D>
#include <osgDB/ReadFile>
#include <osgUtil/CullVisitor>
using namespace osgVerse;

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

/* SkyBox */

SkyBox::SkyBox()
    : osg::Transform()
{
}

SkyBox::SkyBox(const SkyBox& copy, const osg::CopyOp& copyop)
    : osg::Transform(copy, copyop), _skymap(copy._skymap)
{
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

    cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    _skymap = cubemap.get(); initialize();
}

void SkyBox::setEnvironmentMap(osg::Image* image)
{    // TODO: 2D texture with shader
    /*if (image)
    {
        osg::ref_ptr<osg::TextureCubeMap> cubemap = new osg::TextureCubeMap;
        loadVerticalCrossCubeMap(cubemap.get(), image);

        cubemap->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        cubemap->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
        cubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
        cubemap->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        cubemap->setInternalFormat(GL_RGBA16F_ARB);

        _skymap = cubemap.get();
        initialize();
    }*/
}

void SkyBox::initialize()
{
    osg::ref_ptr<osg::StateSet> stateset = new osg::StateSet();
    unsigned int values = osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE;

    osg::ref_ptr<osg::TexGen> tg = new osg::TexGen;
    tg->setMode(osg::TexGen::REFLECTION_MAP);  // FIXME: use shader
    stateset->setTextureAttributeAndModes(0, tg.get(), values);

    osg::TexMat* tm = new osg::TexMat;
    stateset->setTextureAttribute(0, tm);

    stateset->setTextureAttributeAndModes(0, _skymap.get(), values);
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateset->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Depth> depth = new osg::Depth;
    depth->setFunction(osg::Depth::ALWAYS);
    depth->setRange(1.0, 1.0);
    stateset->setAttributeAndModes(depth, values);
    stateset->setRenderBinDetails(-1, "RenderBin");

    osg::ref_ptr<osg::Drawable> drawable = new osg::ShapeDrawable(
        new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 10.0f));
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->setCullingActive(false);
    geode->setStateSet(stateset.get());
    geode->addDrawable(drawable.get());
    geode->setName("SkyBoxGeode");

    setCullCallback(new TexMatCallback(*tm));
    addChild(geode.get());
}

bool SkyBox::computeLocalToWorldMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        matrix.preMult(osg::Matrix::translate(cv->getEyeLocal()));
        return true;
    }
    else
        return osg::Transform::computeLocalToWorldMatrix(matrix, nv);
}

bool SkyBox::computeWorldToLocalMatrix(osg::Matrix& matrix, osg::NodeVisitor* nv) const
{
    if (nv && nv->getVisitorType() == osg::NodeVisitor::CULL_VISITOR)
    {
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(nv);
        matrix.postMult(osg::Matrix::translate(-cv->getEyeLocal()));
        return true;
    }
    else
        return osg::Transform::computeWorldToLocalMatrix(matrix, nv);
}
