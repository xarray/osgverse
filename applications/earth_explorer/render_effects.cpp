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

#include <picojson.h>
#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <pipeline/Pipeline.h>
#include <ui/ImGui.h>
#include <ui/ImGuiComponents.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

const char* finalVertCode = {
    "VERSE_VS_OUT vec4 texCoord; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* finalFragCode = {
    "uniform sampler2D EarthTexture, OceanTexture;\n"
    "VERSE_FS_IN vec4 texCoord; \n"
    "VERSE_FS_OUT vec4 fragColor;\n"

    "void main() {\n"
    "    vec4 sceneColor = VERSE_TEX2D(EarthTexture, texCoord.st);\n"
    "    vec4 oceanColor = VERSE_TEX2D(OceanTexture, texCoord.st);\n"
    "    fragColor = sceneColor;//mix(sceneColor, oceanColor, oceanColor.a); \n"
    "    VERSE_FS_FINAL(fragColor);\n"
    "}\n"
};

class OceanUpdater : public osgGA::GUIEventHandler
{
public:
    OceanUpdater(osgVerse::EarthAtmosphereOcean* e) : _earthData(e) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            _earthData->update(view->getCamera());
            _earthData->updateOcean(view->getCamera());
        }
        return false;
    }

protected:
    osgVerse::EarthAtmosphereOcean* _earthData;
};

std::vector<osg::Camera*> configureEarthRendering(
        osgViewer::View& viewer, osg::Group* root,
        osg::Node* earth, osgVerse::EarthAtmosphereOcean& earthRenderingUtils,
        const std::string& mainFolder, unsigned int mask, int w, int h)
{
    // Initialize earth utilities
    std::vector<osg::Camera*> cameras;
    earthRenderingUtils.create(BASE_DIR + "/textures/transmittance.raw",
                               BASE_DIR + "/textures/irradiance.raw",
                               BASE_DIR + "/textures/sunglare.png",
                               BASE_DIR + "/textures/inscatter.raw");

    // Add earth globe to RTT camera
    osg::ref_ptr<osg::Texture> earthColorBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::ref_ptr<osg::Texture> earthMaskBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::R_INT8, w, h);

    osg::Camera* earthCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, false);
    earthCamera->attach(osg::Camera::COLOR_BUFFER0, earthColorBuffer.get(), 0, 0, false, 16, 4);
    earthCamera->attach(osg::Camera::COLOR_BUFFER1, earthMaskBuffer.get());
    earthCamera->setViewport(0, 0, w, h);
    earthCamera->addChild(earth);

    osg::Texture* defTex0 = osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    osg::Texture* defTex1 = osgVerse::createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
    earthRenderingUtils.applyToGlobe(earthCamera->getOrCreateStateSet(), defTex0, defTex0, defTex1,
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_globe.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_globe.frag.glsl"));
    root->addChild(earthCamera); cameras.push_back(earthCamera);

    // Merge atmosphere with earth color and render them on a screen quad
    osg::ref_ptr<osg::Texture> mergedGlobeBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::Camera* skyAndGlobeCamera = osgVerse::createRTTCamera(
        osg::Camera::COLOR_BUFFER0, mergedGlobeBuffer.get(), NULL, true);
    earthRenderingUtils.applyToAtmosphere(skyAndGlobeCamera->getOrCreateStateSet(), earthColorBuffer.get(),
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "scattering_sky.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "scattering_sky.frag.glsl"));
    skyAndGlobeCamera->setNodeMask(mask); cameras.push_back(skyAndGlobeCamera);
    root->addChild(skyAndGlobeCamera);

    // Add post-processing ocean camera if necessary
    osg::ref_ptr<osg::Texture> oceanColorBuffer =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    osg::Camera* oceanCamera = osgVerse::createRTTCamera(
        osg::Camera::COLOR_BUFFER0, oceanColorBuffer.get(), NULL, false);
    oceanCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    oceanCamera->setProjectionMatrix(osg::Matrix::ortho2D(-1.0, 1.0, -1.0, 1.0));
    oceanCamera->setViewMatrix(osg::Matrix::identity());
    oceanCamera->setNodeMask(mask); cameras.push_back(oceanCamera);

    osg::Geometry* grid = earthRenderingUtils.createOceanGrid(w, h);
    osg::Geode* grideGeode = new osg::Geode; grideGeode->addDrawable(grid);
    oceanCamera->addChild(grideGeode);

    float seaRoughness = 0.0f;
    earthRenderingUtils.applyToOcean(oceanCamera->getOrCreateStateSet(),
        earthMaskBuffer.get(), earthRenderingUtils.createOceanWaves(seaRoughness),
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "global_ocean.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "global_ocean.frag.glsl"));
    root->addChild(oceanCamera);

    // Merge results to an HUD camera to show
    osg::Camera* finalCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0f, 1.0f, true);
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, mergedGlobeBuffer.get());
    finalCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, oceanColorBuffer.get());
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("EarthTexture", (int)0));
    finalCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("OceanTexture", (int)1));
    finalCamera->setNodeMask(mask); cameras.push_back(finalCamera);
    root->addChild(finalCamera);

    osg::ref_ptr<osg::Program> program = new osg::Program;
    {
        osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, finalVertCode);
        osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, finalFragCode);
        program->addShader(vs); program->addShader(fs);
        osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
    }
    finalCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
    viewer.addEventHandler(new OceanUpdater(&earthRenderingUtils));
    return cameras;
}
