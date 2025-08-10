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

static std::string s_dataList[] = {
    "USGS_Earthquake/2020_1.particle",
    "USGS_Earthquake/2020_2.particle",
    "USGS_Earthquake/2021_1.particle",
    "USGS_Earthquake/2021_2.particle",
    "USGS_Earthquake/2022_1.particle",
    "USGS_Earthquake/2022_2.particle",
    "USGS_Earthquake/2023_1.particle",
    "USGS_Earthquake/2023_2.particle",
    "USGS_Earthquake/2024_1.particle",
    "USGS_Earthquake/2024_2.particle",
    "USGS_Earthquake/2025_1.particle"
};
static int s_dataListCount = 11;

class TimelineParticleHandler : public osgGA::GUIEventHandler
{
public:
    TimelineParticleHandler(osgVerse::ParticleSystemU3D* cloud, const std::string& mainFolder)
        : _cloud(cloud), _mainFolder(mainFolder), _dataIndex(0)
    { updateParticleCloud(_mainFolder + "/" + s_dataList[0]); }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP &&
            ea.getKey() == osgGA::GUIEventAdapter::KEY_Tab)
        {
            if (_dataIndex >= s_dataListCount - 1) _dataIndex = 0; else _dataIndex++;
            updateParticleCloud(_mainFolder + "/" + s_dataList[_dataIndex]);
        }
        return false;
    }

protected:
    void updateParticleCloud(const std::string& file)
    {
        std::ifstream in(file, std::ios::in | std::ios::binary);
        if (!in) return;

        osg::ref_ptr<osgVerse::ParticleCloud> pointCloud = new osgVerse::ParticleCloud;
        pointCloud->setInjector([](osgVerse::ParticleSystemU3D& ps, osgVerse::ParticleCloud& cloud)
        {
            osg::Vec4Array* pos = cloud.getPositions(); pos->dirty();
            osg::Vec4Array* color = cloud.getColors(); color->dirty();
            osg::Vec4Array* attr = cloud.getAttributes();
            for (size_t i = 0; i < pos->size(); ++i)
            {
                float value = osg::clampBetween(pow((*attr)[i].x(), 2.0f) * 0.01f, 0.0f, 1.0f);
                tinycolormap::Color c = tinycolormap::GetColor(value, tinycolormap::ColormapType::Viridis);
                (*color)[i] = osg::Vec4(c.r(), c.g(), c.b(), 0.5f);
                (*pos)[i].a() = 20000.0f;
            }
        });
        pointCloud->load(in); in.close();
        _cloud->setPointCloud(pointCloud.get(), true);
    }

    osg::observer_ptr<osgVerse::ParticleSystemU3D> _cloud;
    std::string _mainFolder; int _dataIndex;
};

void configureParticleCloud(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder,
                            unsigned int mask, bool withGeomShader)
{
    osg::ref_ptr<osg::Geode> particleNode = new osg::Geode;
    particleNode->getOrCreateStateSet()->setAttributeAndModes(new osg::Depth(osg::Depth::ALWAYS, 0.0, 1.0, false));
    particleNode->setNodeMask(mask);
    root->addChild(particleNode.get());

    osg::ref_ptr<osg::Image> img1 = osgDB::readImageFile(BASE_DIR + "/textures/water_drop.png");
    osg::ref_ptr<osg::Image> img2 = osgDB::readImageFile(BASE_DIR + "/textures/strip.png");
    osg::ref_ptr<osg::Shader> vs = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "particles.vert.glsl");
    osg::ref_ptr<osg::Shader> fs = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "particles.frag.glsl");
    osg::ref_ptr<osg::Shader> gs = osgDB::readShaderFile(osg::Shader::GEOMETRY, SHADER_DIR + "particles.geom.glsl");

    osg::ref_ptr<osgVerse::ParticleSystemU3D> cloudAll = new osgVerse::ParticleSystemU3D(
        withGeomShader ? osgVerse::ParticleSystemU3D::GPU_GEOMETRY : osgVerse::ParticleSystemU3D::CPU_VERTEX_ATTRIB);
    cloudAll->setTexture(osgVerse::createTexture2D(img1.get()));
    cloudAll->setParticleType(osgVerse::ParticleSystemU3D::PARTICLE_Billboard);
    cloudAll->setBlendingType(osgVerse::ParticleSystemU3D::BLEND_Additive);
    cloudAll->setGravityScale(0.0f); cloudAll->setAspectRatio(16.0 / 9.0);
    cloudAll->linkTo(particleNode.get(), true, vs.get(), fs.get(), gs.get());
    viewer.addEventHandler(new TimelineParticleHandler(cloudAll.get(), mainFolder));
}
