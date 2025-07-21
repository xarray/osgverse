#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <3rdparty/tinycolormap.hpp>
#include <modeling/Math.h>
#include <animation/ParticleEngine.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

void configureParticleCloud(osg::Group* root, const std::string& mainFolder, unsigned int mask, bool withGeomShader)
{
    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    particleNode->setNodeMask(mask);
    root->addChild(particleNode.get());

    osg::ref_ptr<osg::Image> img1 = osgDB::readImageFile(BASE_DIR + "/textures/water_drop.png");
    osg::ref_ptr<osg::Image> img2 = osgDB::readImageFile(BASE_DIR + "/textures/strip.png");
    osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
    osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");
    osg::ref_ptr<osg::Shader> gs = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "particles.geom.glsl");

    std::ifstream in1(mainFolder + "/newzealand.particle", std::ios::in | std::ios::binary);
    if (!!in1)
    {
        osg::ref_ptr<osgVerse::ParticleCloud> pointCloud = new osgVerse::ParticleCloud;
        pointCloud->setInjector([](osgVerse::ParticleSystemU3D& ps, osgVerse::ParticleCloud& cloud)
        {
            osg::Vec4Array* pos = cloud.getPositions(); pos->dirty();
            osg::Vec4Array* color = cloud.getColors(); color->dirty();
            osg::Vec4Array* attr = cloud.getAttributes();
            for (size_t i = 0; i < pos->size(); ++i)
            {
                tinycolormap::Color c = tinycolormap::GetColor((*attr)[i].x(), tinycolormap::ColormapType::Viridis);
                (*color)[i] = osg::Vec4(c.r(), c.g(), c.b(), 0.01f);
                (*pos)[i].a() = 100.0f;
            }
        });
        pointCloud->load(in1); in1.close();

        osg::ref_ptr<osgVerse::ParticleSystemU3D> cloud = new osgVerse::ParticleSystemU3D(
            withGeomShader ? osgVerse::ParticleSystemU3D::GPU_GEOMETRY : osgVerse::ParticleSystemU3D::CPU_VERTEX_ATTRIB);
        cloud->setTexture(osgVerse::createTexture2D(img1.get()));
        cloud->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        cloud->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        cloud->setPointCloud(pointCloud.get(), true);
        cloud->setGravityScale(0.0f); cloud->setAspectRatio(16.0 / 9.0);
        cloud->linkTo(particleNode.get(), true, vs.get(), fs.get(), gs.get());
    }

    std::ifstream in2(mainFolder + "/global_earthquake.particle", std::ios::in | std::ios::binary);
    if (!!in2)
    {
        osg::ref_ptr<osgVerse::ParticleCloud> pointCloud = new osgVerse::ParticleCloud;
        pointCloud->setInjector([withGeomShader](osgVerse::ParticleSystemU3D& ps, osgVerse::ParticleCloud& cloud)
        {
            osg::Vec4Array* pos = cloud.getPositions(); pos->dirty();
            osg::Vec4Array* color = cloud.getColors(); color->dirty();
            osg::Vec4Array* attr = cloud.getAttributes();
            for (size_t i = 0; i < pos->size(); ++i)
            {
                float value = osg::clampBetween(pow((*attr)[i].x(), 2.0f) * 0.01f, 0.0f, 1.0f);
                tinycolormap::Color c = tinycolormap::GetColor(value, tinycolormap::ColormapType::Jet);
                (*color)[i] = osg::Vec4(c.r(), c.g(), c.b(), 0.5f);
                if (withGeomShader) (*pos)[i].a() = 1000.0f * (value * 10.0f + 1.0f);
            }
        });
        pointCloud->load(in2); in2.close();

        osg::ref_ptr<osgVerse::ParticleSystemU3D> cloud = new osgVerse::ParticleSystemU3D(
            withGeomShader ? osgVerse::ParticleSystemU3D::GPU_GEOMETRY : osgVerse::ParticleSystemU3D::CPU_VERTEX_ATTRIB);
        cloud->setTexture(osgVerse::createTexture2D(img1.get()));
        cloud->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
        cloud->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
        cloud->setPointCloud(pointCloud.get(), true);
        cloud->setGravityScale(0.0f); cloud->setAspectRatio(16.0 / 9.0);
        cloud->linkTo(particleNode.get(), true, vs.get(), fs.get(), gs.get());
    }
}
