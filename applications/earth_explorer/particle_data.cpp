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

void configureParticleCloud(osg::Group* root, const std::string& cloudFile, unsigned int mask)
{
    osg::ref_ptr<osgVerse::ParticleCloud> pointCloud = new osgVerse::ParticleCloud;
    if (cloudFile.empty())
    {
        for (int lon = -180; lon < 180; lon++)
            for (int lat = -90; lat < 90; ++lat)
            {
                osg::Vec3d pos = osgVerse::Coordinate::convertLLAtoECEF(
                    osg::Vec3d(osg::inDegrees((double)lat), osg::inDegrees((double)lon), 0.0));
                pointCloud->add(pos, osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f), osg::Vec3(),
                                osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f), 10000.0f);
            }
    }
    else
    {
        std::ifstream in(cloudFile.c_str(), std::ios::in | std::ios::binary);
        pointCloud->setInjector([](osgVerse::ParticleSystemU3D& ps, osgVerse::ParticleCloud& cloud)
        {
            osg::Vec4Array* pos = cloud.getPositions(); pos->dirty();
            osg::Vec4Array* color = cloud.getColors(); color->dirty();
            osg::Vec4Array* attr = cloud.getAttributes();
            for (size_t i = 0; i < pos->size(); ++i)
            {
                tinycolormap::Color c = tinycolormap::GetColor((*attr)[i].x(), tinycolormap::ColormapType::Viridis);
                (*color)[i] = osg::Vec4(c.r(), c.g(), c.b(), 0.5f);
                (*pos)[i].a() = 100.0f;
            }
        });
        pointCloud->load(in); in.close();
    }

    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    particleNode->setNodeMask(mask);
    root->addChild(particleNode.get());

    osg::ref_ptr<osg::Image> img = osgDB::readImageFile(BASE_DIR + "/textures/water_drop.png");
    osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
    osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");
    osg::ref_ptr<osg::Shader> gs = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "particles.geom.glsl");
    osgVerse::ParticleSystemU3D::UpdateMethod method = osgVerse::ParticleSystemU3D::GPU_GEOMETRY;

    osg::ref_ptr<osgVerse::ParticleSystemU3D> cloud = new osgVerse::ParticleSystemU3D(method);
    cloud->setTexture(osgVerse::createTexture2D(img.get()));
    cloud->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
    cloud->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
    cloud->setPointCloud(pointCloud.get(), true);
    cloud->setGravityScale(0.0f); cloud->setAspectRatio(16.0 / 9.0);
    cloud->linkTo(particleNode.get(), true, vs.get(), fs.get(), gs.get());
}
