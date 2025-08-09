#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <pipeline/Utilities.h>
#include <pipeline/Pipeline.h>
#include <pipeline/SkyBox.h>
#include <pipeline/Drawer2D.h>

const char* uiVertCode = {
    "VERSE_VS_OUT vec4 texCoord, color; \n"
    "void main() {\n"
    "    texCoord = osg_MultiTexCoord0; color = osg_Color; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* uiFragCode = {
    "uniform sampler2D uiTexture, overlayTexture;\n"
    "VERSE_FS_IN vec4 texCoord, color; \n"
    "#ifdef VERSE_GLES3\n"
    "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
    "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
    "#endif\n"

    "void main() {\n"
    "    vec4 uiColor = VERSE_TEX2D(uiTexture, texCoord.st);\n"
    "    vec4 overlay = VERSE_TEX2D(overlayTexture, vec2(texCoord.s, 1.0 - texCoord.t));\n"
    "    vec4 finalColor = mix(uiColor, overlay, overlay.a);\n"

    "#ifdef VERSE_GLES3\n"
    "    fragColor = finalColor; \n"
    "    fragOrigin = vec4(1.0); \n"
    "#else\n"
    "    gl_FragData[0] = finalColor; \n"
    "    gl_FragData[1] = vec4(1.0); \n"
    "#endif\n"
    "}\n"
};

class UIHandler : public osgGA::GUIEventHandler
{
public:
    UIHandler(osgVerse::Drawer2D* d, const std::string& mainFolder, int w, int h)
        : _drawer(d), _width(w), _height(h)
    {
        _selected = osgDB::readImageFile(mainFolder + "/ui/selected.png");
        _compass0 = osgDB::readImageFile(mainFolder + "/ui/compass0.png");
        _compass1 = osgDB::readImageFile(mainFolder + "/ui/compass1.png");
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME) updateOverlay();
        return false;
    }

    void updateOverlay()
    {
        //_drawer->start(false);
        _drawer->startInThread([=](osgVerse::Drawer2D* drawer)
        {
            float wCell = _width / 32.0f, hCell = _height / 32.0f;
            osgVerse::DrawerStyleData style(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f), true);
            drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

#define DRAW_TEXT(text, x, y) { \
            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, 20.0f); \
            drawer->drawUtf8Text(osg::Vec2(wCell * x, hCell * y) + osg::Vec2(bbox[0], bbox[1] + bbox[3]), \
                                 20.0f, text, "", style); }
#define DRAW_TEXT_MID(text, x, y) { \
            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, 20.0f); \
            drawer->drawUtf8Text(osg::Vec2(wCell * x - bbox[2] * 0.5f, hCell * y) + \
                                 osg::Vec2(bbox[0], bbox[1] + bbox[3]), 20.0f, text, "", style); }

            // Data lists
            DRAW_TEXT("TILES", 1.4f, 5.9f);
            DRAW_TEXT("Global Night Lighting", 1.5f, 7.0f);
            DRAW_TEXT("Global Population Distribution", 1.5f, 8.0f);
            DRAW_TEXT("ERA5 Land 2m Temperature", 1.5f, 9.0f);
            DRAW_TEXT("ERA5 Lake Total Temperature", 1.5f, 10.0f);
            DRAW_TEXT("ERA5 Lake Ice Temperature", 1.5f, 11.0f);

            DRAW_TEXT("VOLUMES", 1.4f, 12.8f);
            DRAW_TEXT("Kerry Volume Data", 1.5f, 14.0f);
            DRAW_TEXT("Parhaka Volume Data", 1.5f, 15.0f);
            DRAW_TEXT("Waipuku Volume Data", 1.5f, 16.0f);

            DRAW_TEXT("CITIES", 1.4f, 17.8f);
            DRAW_TEXT("Beijing", 1.5f, 19.0f); DRAW_TEXT("New York", 5.0f, 19.0f);
            DRAW_TEXT("Capetown", 1.5f, 20.0f); DRAW_TEXT("Paris", 5.0f, 20.0f);
            DRAW_TEXT("Hangzhou", 1.5f, 21.0f); DRAW_TEXT("Rio", 5.0f, 21.0f);
            DRAW_TEXT("London", 1.5f, 22.0f); DRAW_TEXT("Shanghai", 5.0f, 22.0f);
            DRAW_TEXT("Nanjing", 1.5f, 23.0f); DRAW_TEXT("Sydney", 5.0f, 23.0f);

            DRAW_TEXT("TIMELINE", 1.4f, 24.8f);
            DRAW_TEXT("USGS Earthquake", 1.5f, 26.0f);
            DRAW_TEXT("GPlates Motion", 1.5f, 27.0f);

            // Selected item / button
            // TODO

            // LLA, compass and scale display
            DRAW_TEXT("140°40'20\"E", 25.0f, 28.5f);
            DRAW_TEXT("40°40'20\"N", 27.5f, 28.5f);
            DRAW_TEXT("9999m", 30.0f, 28.5f);

            DRAW_TEXT_MID("40°40'NE", 29.0f, 9.5f);
            DRAW_TEXT_MID("SCALE", 29.0f, 10.5f);
            
            // Compass
            osg::Vec2 pos(wCell * 28.0f, hCell * 6.0f);
            drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 2.0f, hCell * 32.0f / 9.0f),
                                  0.0f, 0.0f, osgVerse::DrawerStyleData(_compass0.get()));
            drawer->translate(-pos - osg::Vec2(wCell, hCell * 16.0f / 9.0f), false);
            drawer->rotate(osg::inDegrees(30.0f), true);
            drawer->translate(pos + osg::Vec2(wCell, hCell * 16.0f / 9.0f), true);
            drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 2.0f, hCell * 32.0f / 9.0f),
                                  0.0f, 0.0f, osgVerse::DrawerStyleData(_compass1.get()));
        }, false);
        _drawer->finish();
    }

protected:
    osg::ref_ptr<osgVerse::Drawer2D> _drawer;
    osg::ref_ptr<osg::Image> _selected;
    osg::ref_ptr<osg::Image> _compass0, _compass1;
    int _width, _height;
};

void configureUI(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder, int w, int h)
{
    osg::ref_ptr<osg::Image> background = osgDB::readImageFile(mainFolder + "/ui/main.png");
    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->loadFont("default", mainFolder + "/ui/pingfang.ttf");
    drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));

    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0, 1.0, true);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->setClearColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(background.get()));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, osgVerse::createTexture2D(drawer.get()));
    hudCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER, 20000);

    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, uiVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, uiFragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("UI_VS"); program->addShader(vs);
    fs->setName("UI_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME
    hudCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
    hudCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("uiTexture", (int)0));
    hudCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("overlayTexture", (int)1));

    root->addChild(hudCamera);
    viewer.addEventHandler(new UIHandler(drawer.get(), mainFolder, w, h));
}
