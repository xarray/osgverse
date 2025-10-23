#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/TileCallback.h>
#include <pipeline/Utilities.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Drawer2D.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>

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
        : _drawer(d), _mainFolder(mainFolder), _width(w), _height(h)
    {
        _selected = osgDB::readImageFile(mainFolder + "/UI/selected.png");
        _unselected = osgDB::readImageFile(mainFolder + "/UI/unselected.png");
        _compass = osgDB::readImageFile(mainFolder + "/UI/compass.png");
        _prop0 = osgDB::readImageFile(mainFolder + "/UI/prop0.png");
        _prop1 = osgDB::readImageFile(mainFolder + "/UI/prop1.png");

        std::string cityString, propString, rev0, rev1;
        std::vector<unsigned char> result1 = osgVerse::loadFileData(mainFolder + "/UI/cities_cn.txt", rev0, rev1);
        cityString.assign(result1.begin(), result1.end()); osgDB::split(cityString, _cityContent, '\n');

        std::vector<unsigned char> result2 = osgVerse::loadFileData(mainFolder + "/UI/props_cn.txt", rev0, rev1);
        propString.assign(result2.begin(), result2.end()); osgDB::split(propString, _propContent, '\n');
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        osgVerse::EarthManipulator* manipulator =
            static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
        if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE || ea.getEventType() == osgGA::GUIEventAdapter::DRAG ||
            ea.getEventType() == osgGA::GUIEventAdapter::SCROLL || ea.getEventType() == osgGA::GUIEventAdapter::RELEASE)
        {
            updateOverlay(view, manipulator, ea.getXnormalized(), ea.getYnormalized());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            if (view->getFrameStamp()->getFrameNumber() < 1)
                updateOverlay(view, manipulator, ea.getXnormalized(), ea.getYnormalized());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH &&
            ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) _buttonState = 1;
        else if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
        {
            const osgDB::Options* ev = dynamic_cast<const osgDB::Options*>(ea.getUserData());
            std::string command = ev ? ev->getOptionString() : "";

            std::vector<std::string> commmandPair; osgDB::split(command, commmandPair, '/');
            if (commmandPair.front() == "button" && commmandPair.back() == "info")
            {
                if (!_cityInfo)
                {
                    for (std::map<std::string, bool>::iterator itr = _itemSelection.begin();
                         itr != _itemSelection.end(); ++itr)
                    {
                        if (!itr->second) continue; if (itr->first.find("item/") == std::string::npos) continue;
                        size_t pos0 = itr->first.find("item/") + 5, pos1 = itr->first.find("_");
                        std::string imgName = "info_" + itr->first.substr(pos0, pos1 - pos0) + ".png";
                        _cityInfo = osgDB::readImageFile(_mainFolder + "/UI/" + imgName); break;
                    }
                }
                else _cityInfo = NULL;
            }
            updateOverlay(view, manipulator, ea.getXnormalized(), ea.getYnormalized());
        }
        return false;
    }

    void updateOverlay(osgViewer::View* view, osgVerse::EarthManipulator* manipulator, float xx, float yy)
    {
        //_drawer->start(false);
        _drawer->startInThread([=](osgVerse::Drawer2D* drawer)
        {
            float wCell = _width / 32.0f, hCell = _height / 32.0f;
            std::map<osg::Vec4, std::string> hoverableItems;
            osgVerse::DrawerStyleData style(osg::Vec4f(1.0f, 1.0f, 1.0f, 1.0f), true);
            osgVerse::DrawerStyleData selected(osg::Vec4(0.6f, 0.6f, 0.8f, 0.6f), true);
            osgVerse::DrawerStyleData pushed(osg::Vec4(0.2f, 0.2f, 0.4f, 0.9f), true);
            drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

#define ITEM_RECT(x1, y1, x2, y2) hoverableItems[osg::Vec4(wCell * (x1), hCell * (y1), wCell * (x2), hCell * (y2))]
#define IN_ITEM(x, y, r) ((x) <= (r)[2] && (x) >= (r)[0] && (y) <= (r)[3] && (y) >= (r)[1])

#define DRAW_TEXT_S(text, x, y, s) { \
            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, s); \
            drawer->drawUtf8Text(osg::Vec2(wCell * (x), hCell * (y)) + osg::Vec2(bbox[0], bbox[1] + bbox[3]), \
                                 s, text, "", style); }
#define DRAW_TEXT(text, x, y) DRAW_TEXT_S(text, x, y, 20.0f)
#define DRAW_TEXT_MID(text, x, y) { \
            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, 20.0f); \
            drawer->drawUtf8Text(osg::Vec2(wCell * (x) - bbox[2] * 0.5f, hCell * (y)) + \
                                 osg::Vec2(bbox[0], bbox[1] + bbox[3]), 20.0f, text, "", style); }

            // Data lists
            float as = 16.0f / 9.0f; bool bSelected = false, vSelected = false, segSelected = false;
            if (!_cityContent.empty())
            {
                DRAW_TEXT_S(_cityContent[0], 1.5f, 2.4f, 40.0f);
                for (size_t i = 1; i < _cityContent.size(); ++i)
                {
                    std::vector<std::string> cmdAndText; size_t k = i + 5;
                    osgDB::split(_cityContent[i], cmdAndText, ':');
                    if (k > 29) break;  // too many items  // TODO: scroll the list?

                    std::string itemName = "item/" + cmdAndText.front();
                    DRAW_TEXT(cmdAndText.back(), 1.0f, (float)k);
                    ITEM_RECT(1.0f, (float)k, 9.0f, (float)(k + 1)) = itemName;

                    bool selected = _itemSelection[itemName];
                    if (selected)
                    {
                        if (itemName.find("seg") != std::string::npos) segSelected = true;
                        else if (itemName.find("vehicles") != std::string::npos) vSelected = true;
                        else if (itemName.find("buildings") != std::string::npos) bSelected = true;
                    }

                    osg::Vec2 pos(wCell * 8.2f, hCell * (0.2f + (float)k));
                    drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 0.3f, hCell * 0.3f * as), 0.0f, 0.0f,
                                          osgVerse::DrawerStyleData(selected ? _selected.get() : _unselected.get()));
                }
            }

            // Property dialogs
            if (_propContent.size() > 5)
            {
                float vStart = 5.0f;
                if (vSelected)
                {
                    int count0 = 1000, count1 = 402, count2 = 598;
                    drawer->drawRectangle(osg::Vec4(wCell * 24.0f, hCell * vStart, wCell * 6.0f, hCell * 3.5f * as),
                                          0.0f, 0.0f, osgVerse::DrawerStyleData(_prop0.get()));
                    DRAW_TEXT_S(_propContent[0], 24.2f, vStart, 25.0f);
                    DRAW_TEXT(_propContent[1], 24.7f, vStart + 1.5f);
                    DRAW_TEXT(_propContent[2] + std::to_string(count0), 25.2f, vStart + 2.6f);
                    DRAW_TEXT(_propContent[3] + std::to_string(count1), 25.2f, vStart + 3.6f);
                    DRAW_TEXT(_propContent[4] + std::to_string(count2), 25.2f, vStart + 4.6f);
                    vStart += 7.0f;
                }

                if (bSelected)
                {
                    drawer->drawRectangle(osg::Vec4(wCell * 24.0f, hCell * vStart, wCell * 6.0f, hCell * 8.0f * as),
                                          0.0f, 0.0f, osgVerse::DrawerStyleData(_prop1.get()));
                    DRAW_TEXT_S(_propContent[5], 24.2f, vStart, 25.0f);
                    for (size_t i = 6; i < _propContent.size(); ++i)
                    {
                        std::vector<std::string> textAndColor; osgDB::split(_propContent[i], textAndColor, ':');
                        osg::Vec4 color = osgVerse::Auxiliary::hexColorToRGB(osgVerse::Auxiliary::trim(textAndColor.back()));
                        drawer->drawRectangle(osg::Vec4(wCell * 24.5f, hCell * (vStart + 1.5f + 0.35f),
                                              wCell * 0.3f, hCell * 0.3f * as), 0.0f, 0.0f,
                                              osgVerse::DrawerStyleData(color, true));
                        DRAW_TEXT(textAndColor.front(), 25.2f, vStart + 1.5f); vStart += 1.0f;
                    }
                }
            }

            // Info popup dialog
            if (_cityInfo.valid())
            {
                drawer->drawRectangle(osg::Vec4(wCell * 10.0f, hCell * 7.0f, wCell * 12.0f, hCell * 11.0f * as),
                                      0.0f, 0.0f, osgVerse::DrawerStyleData(_cityInfo.get()));
            }

            // Buttons
            ITEM_RECT(30.6f, 18.5f, 31.35f, 19.8f) = "button/light";
            ITEM_RECT(30.6f, 19.9f, 31.35f, 21.2f) = "button/go_home";
            ITEM_RECT(30.6f, 21.3f, 31.35f, 22.6f) = "button/auto_rotate";
            ITEM_RECT(30.6f, 22.7f, 31.35f, 24.0f) = "button/ocean";
            ITEM_RECT(30.6f, 24.1f, 31.35f, 25.4f) = "button/info";
            ITEM_RECT(30.6f, 26.0f, 31.35f, 27.5f) = "button/zoom_in";
            ITEM_RECT(30.6f, 27.4f, 31.35f, 28.7f) = "button/zoom_out";

            ITEM_RECT(28.4f, 3.0f, 29.0f, 4.0f) = "button/about?";
            ITEM_RECT(29.4f, 3.0f, 30.0f, 4.0f) = "button/ch_en";
            DRAW_TEXT("CN", 29.5f, 3.0f);
            
            float x = (xx * 0.5f + 0.5f) * _width, y = (-yy * 0.5f + 0.5f) * _height;
            for (std::map<osg::Vec4, std::string>::iterator it = hoverableItems.begin();
                 it != hoverableItems.end(); ++it)
            {
                if (!IN_ITEM(x, y, it->first)) continue;
                osg::Vec4 rect(it->first.x(), it->first.y(),
                               it->first.z() - it->first.x(), it->first.w() - it->first.y());
                drawer->drawRectangle(rect, 1.0f, 1.0f, _buttonState > 0 ? pushed : selected);

                // Run command when clicked (buttonState = 1)
                if (_buttonState == 1)
                {
                    _itemSelection[it->second] = !_itemSelection[it->second];
                    view->getEventQueue()->userEvent(new osgDB::Options(it->second));
                }
                break;
            }
            if (_buttonState > 0) _buttonState = 0;

            // LLA, compass and scale display
            double northRadians = 0.0, widthScale = 0.0;
            osg::Camera* camera = view->getCamera();
            if (camera && camera->getViewport())
            {
                osg::Vec3d worldNorth(0.0, 0.0, osg::WGS_84_RADIUS_POLAR);
                osg::Vec3d viewNorth = worldNorth * camera->getViewMatrix();
                osg::Vec2d compassDirection(viewNorth.x(), viewNorth.y());
                compassDirection.normalize();
                northRadians = std::atan2(compassDirection.y(), compassDirection.x());
                //DRAW_TEXT_MID(radiansToCompassHeading(northRadians), 29.0f, 9.5f);

                double fovy, aspect, nearPlane, farPlane, distance;
                distance = (osg::Vec3d() * camera->getViewMatrix()).length() - osg::WGS_84_RADIUS_POLAR;
                camera->getProjectionMatrixAsPerspective(fovy, aspect, nearPlane, farPlane);
                double heightNear = 2.0 * distance * tan(osg::inDegrees(fovy) / 2.0);
                widthScale = heightNear / camera->getViewport()->height();
                if (widthScale < 1.0)
                {
                    std::string wStr = std::to_string((int)(widthScale * 100.0) * 0.01f);
                    //DRAW_TEXT_MID("S = " + wStr + "m/pixel", 29.0f, 10.5f);
                }
                else
                {
                    int wValue = (int)widthScale;
                    std::string wStr = (wValue > 1000.0) ? std::to_string((int)(wValue * 0.001)) + "km/pixel"
                                                         : std::to_string((int)wValue) + "m/pixel";
                    //DRAW_TEXT_MID("S = " + wStr, 29.0f, 10.5f);
                }
            }

            if (manipulator)
            {
                osg::Vec3d lla = manipulator->getLatestPosition();
                DRAW_TEXT(radiansToDMS(lla[1], 'E', 'W'), 23.5f, 29.5f);
                DRAW_TEXT(radiansToDMS(lla[0], 'N', 'S'), 26.0f, 29.5f);
                if (widthScale < 10000) DRAW_TEXT(std::to_string((int)lla[2]) + "m", 28.5f, 29.5f);
            }

            // Compass
            osg::Vec2 pos(wCell * 30.75f, hCell * 29.0f);
            drawer->translate(-pos - osg::Vec2(wCell * 0.25f, hCell * 0.25f * as), false);
            drawer->rotate(osg::PI_2 - northRadians, true);
            drawer->translate(pos + osg::Vec2(wCell * 0.25f, hCell * 0.25f * as), true);
            drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 0.5f, hCell * 0.5f * as),
                                  0.0f, 0.0f, osgVerse::DrawerStyleData(_compass.get()));
        }, false);
        _drawer->finish();
    }

    static std::string radiansToCompassHeading(double radians)
    {
        double degrees = radians * (180.0 / osg::PI);
        while (degrees < 0) degrees += 360.0;
        while (degrees >= 360) degrees -= 360.0;

        int d = static_cast<int>(degrees);
        double remaining = (degrees - d) * 60.0;
        int m = static_cast<int>(remaining);

        std::string direction; std::ostringstream oss;
        if (degrees >= 0 && degrees < 90) direction = "NE";
        else if (degrees >= 90 && degrees < 180) direction = "NW";
        else if (degrees >= 180 && degrees < 270) direction = "SW";
        else direction = "SE";
        oss << d << "°" << m << "'" << direction;
        return oss.str();
    }

    static std::string radiansToDMS(double radians, char ch0, char ch1)
    {
        double degrees = radians * (180.0 / osg::PI);
        char direction = (degrees >= 0) ? ch0 : ch1;
        degrees = std::abs(degrees);

        int d = static_cast<int>(degrees); double remaining = (degrees - d) * 60;
        int m = static_cast<int>(remaining); double s = (remaining - m) * 60;
        std::ostringstream oss;
        oss << d << "°" << m << "'" << std::fixed << std::setprecision(2) << s << "\"" << direction;
        return oss.str();
    }

protected:
    std::map<std::string, bool> _itemSelection;
    std::vector<std::string> _cityContent, _propContent;
    osg::ref_ptr<osgVerse::Drawer2D> _drawer;
    osg::ref_ptr<osg::Image> _selected, _unselected, _compass;
    osg::ref_ptr<osg::Image> _cityInfo, _prop0, _prop1;
    std::string _mainFolder; int _width, _height, _buttonState;
};

osg::Camera* configureUI(osgViewer::View& viewer, osg::Group* root,
                         const std::string& mainFolder, int w, int h)
{
    osg::ref_ptr<osg::Image> background = osgDB::readImageFile(mainFolder + "/UI/main.png");
    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->loadFont("default", mainFolder + "/UI/pingfang.ttf");
    drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));

    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0, 1.0, true);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER, 20000);
    hudCamera->setClearColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(background.get()));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, osgVerse::createTexture2D(drawer.get()));

    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, uiVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, uiFragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("UI_VS"); program->addShader(vs);
    fs->setName("UI_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);
    hudCamera->getOrCreateStateSet()->setAttributeAndModes(program.get());
    hudCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("uiTexture", (int)0));
    hudCamera->getOrCreateStateSet()->addUniform(new osg::Uniform("overlayTexture", (int)1));

    viewer.addEventHandler(new UIHandler(drawer.get(), mainFolder, w, h));
    root->addChild(hudCamera); return hudCamera;
}
