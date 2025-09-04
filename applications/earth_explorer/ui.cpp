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
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/TileCallback.h>
#include <pipeline/Utilities.h>
#include <pipeline/Pipeline.h>
#include <pipeline/SkyBox.h>
#include <pipeline/Drawer2D.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <iostream>

extern std::mutex commandMutex;
extern std::set<std::string> commandList;
extern osg::ref_ptr<osg::Texture> finalBuffer2;
extern osg::Vec2 global_volumeRange;
extern int global_particleIndex;

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

struct PaleoCallback : public osgVerse::TileManager::DynamicTileCallback
{
    virtual bool shouldMorph(osgVerse::TileCallback& cb) const
    { return mapImage.valid() ? (morphedTiles.find(cb.getName()) == morphedTiles.end()) : false; }

    virtual bool updateEntireTileGeometry(osgVerse::TileCallback& cb, osg::Geometry* geom)
    {
        //std::cout << "Updating " << cb.getName() << "\n";
        morphedTiles.insert(cb.getName()); return false;
    }

    virtual osg::Vec3 updateTileVertex(osgVerse::TileCallback& cb, double lat, double lon)
    {
        double s = 180.0 * (lat + osg::PI_2) / osg::PI, t = 360.0 * (lon + osg::PI) * 0.5 / osg::PI;
        double s0 = floor(s), s1 = ceil(s), t0 = floor(t), t1 = ceil(t);
        osg::Vec4 c00 = mapImage->getColor((int)s0, (int)t0), c10 = mapImage->getColor((int)s1, (int)t0);
        osg::Vec4 c01 = mapImage->getColor((int)s0, (int)t1), c11 = mapImage->getColor((int)s1, (int)t1);

        float c = c00[0] * (s - s0) * (t - t0) + c10[0] * (s1 - s) * (t - t0) +
                  c11[0] * (s1 - s) * (t1 - t) + c01[0] * (s - s0) * (t1 - t);
        c = c * cb.getElevationScale();
        return osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3d(lat, lon, c));
    }

    PaleoCallback(osg::Image* img) : mapImage(img) {}
    osg::ref_ptr<osg::Image> mapImage;
    std::set<std::string> morphedTiles;
};

class UIHandler : public osgGA::GUIEventHandler
{
public:
    UIHandler(osgVerse::Drawer2D* d, const std::string& mainFolder, int w, int h)
        : _drawer(d), _width(w), _height(h), _volumeMode(0)
    {
        _selected = osgDB::readImageFile(mainFolder + "/ui/selected.png");
        _volume = osgDB::readImageFile(mainFolder + "/ui/colormap-jet.png");
        _compass0 = osgDB::readImageFile(mainFolder + "/ui/compass0.png");
        _compass1 = osgDB::readImageFile(mainFolder + "/ui/compass1.png");

        osgDB::DirectoryContents contents = osgDB::getDirectoryContents(mainFolder + "/paleo");
        for (size_t i = 0; i < contents.size(); ++i)
        {
            std::string fileName = osgDB::getStrippedName(contents[i]);
            std::vector<std::string> parts; osgDB::split(fileName, parts, '_');
            if (parts.empty()) continue;

            size_t index = parts.back().find("Ma");
            int age = atoi(parts.back().substr(0, index).c_str());
            _paleoMaps[-age] = mainFolder + "/paleo/" + contents[i];
            _paleoAgeList.push_back(-age);
        }

        _paleoCallback = new PaleoCallback(NULL); _paleoIndex = 0;
        osgVerse::TileManager::instance()->setDynamicCallback(_paleoCallback.get());
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        osgVerse::EarthManipulator* manipulator =
            static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
        if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE ||
            ea.getEventType() == osgGA::GUIEventAdapter::DRAG ||
            ea.getEventType() == osgGA::GUIEventAdapter::SCROLL)
        {
            updateOverlay(view->getCamera(), manipulator, ea.getXnormalized(), ea.getYnormalized());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH &&
                 ea.getButtonMask() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON) _buttonState = 1;
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F1)
            {
                if (_paleoIndex > 0) _paleoIndex--;
                int age = _paleoAgeList[_paleoIndex]; std::string file = _paleoMaps[age];
                std::ifstream fin(file.c_str(), std::ios::in | std::ios::binary);
                _paleoCallback->mapImage = createImageFromPaleo(fin);
                _paleoCallback->morphedTiles.clear();
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_F2)
            {
                if (_paleoIndex < _paleoAgeList.size() - 1) _paleoIndex++;
                int age = _paleoAgeList[_paleoIndex]; std::string file = _paleoMaps[age];
                std::ifstream fin(file.c_str(), std::ios::in | std::ios::binary);
                _paleoCallback->mapImage = createImageFromPaleo(fin);
                _paleoCallback->morphedTiles.clear();
            }
        }
        return false;
    }

    void updateOverlay(osg::Camera* camera, osgVerse::EarthManipulator* manipulator,
                       float xx, float yy)
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

#define ITEM_RECT(x1, y1, x2, y2) hoverableItems[osg::Vec4(wCell * x1, hCell * y1, wCell * x2, hCell * y2)]
#define IN_ITEM(x, y, r) (x <= (r)[2] && x >= (r)[0] && y <= (r)[3] && y >= (r)[1])

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

            ITEM_RECT(1.5f, 7.0f, 7.5f, 8.0f) = "layers/Night_Lighting";
            ITEM_RECT(1.5f, 8.0f, 7.5f, 9.0f) = "layers/Population_Distribution";
            ITEM_RECT(1.5f, 9.0f, 7.5f, 10.0f) = "layers/ERA5_Land_2m_Temperature";
            ITEM_RECT(1.5f, 10.0f, 7.5f, 11.0f) = "layers/ERA5_Lake_Total_Temperature";
            ITEM_RECT(1.5f, 11.0f, 7.5f, 12.0f) = "layers/ERA5_Lake_Ice_Temperature";

            DRAW_TEXT("VOLUMES", 1.4f, 12.8f);
            DRAW_TEXT("Kerry Volume Data", 1.5f, 14.0f);
            DRAW_TEXT("Parhaka Volume Data", 1.5f, 15.0f);
            DRAW_TEXT("Waipuku Volume Data", 1.5f, 16.0f);

            ITEM_RECT(1.5f, 14.0f, 7.5f, 15.0f) = "vdb/kerry";
            ITEM_RECT(1.5f, 15.0f, 7.5f, 16.0f) = "vdb/parhaka";
            ITEM_RECT(1.5f, 16.0f, 7.5f, 17.0f) = "vdb/waipuku";

            DRAW_TEXT("CITIES", 1.4f, 17.8f);
            DRAW_TEXT("Beijing", 1.5f, 19.0f); DRAW_TEXT("New York", 5.0f, 19.0f);
            DRAW_TEXT("Capetown", 1.5f, 20.0f); DRAW_TEXT("Paris", 5.0f, 20.0f);
            DRAW_TEXT("Hangzhou", 1.5f, 21.0f); DRAW_TEXT("Rio", 5.0f, 21.0f);
            DRAW_TEXT("London", 1.5f, 22.0f); DRAW_TEXT("Shanghai", 5.0f, 22.0f);
            DRAW_TEXT("Nanjing", 1.5f, 23.0f); DRAW_TEXT("Sydney", 5.0f, 23.0f);

            ITEM_RECT(1.5f, 19.0f, 4.0f, 20.0f) = "cities/beijing.json";
            ITEM_RECT(1.5f, 20.0f, 4.0f, 21.0f) = "cities/capetown.json";
            ITEM_RECT(1.5f, 21.0f, 4.0f, 22.0f) = "cities/hangzhou.json";
            ITEM_RECT(1.5f, 22.0f, 4.0f, 23.0f) = "cities/london.json";
            ITEM_RECT(1.5f, 23.0f, 4.0f, 24.0f) = "cities/nanjing.json";
            ITEM_RECT(5.0f, 19.0f, 7.5f, 20.0f) = "cities/newyork.json";
            ITEM_RECT(5.0f, 20.0f, 7.5f, 21.0f) = "cities/paris.json";
            ITEM_RECT(5.0f, 21.0f, 7.5f, 22.0f) = "cities/riodejaneiro.json";
            ITEM_RECT(5.0f, 22.0f, 7.5f, 23.0f) = "cities/shanghai.json";
            ITEM_RECT(5.0f, 23.0f, 7.5f, 24.0f) = "cities/sydney.json";

            DRAW_TEXT("TIMELINE", 1.4f, 24.8f);
            DRAW_TEXT("USGS Earthquake", 1.5f, 26.0f);
            //DRAW_TEXT("GPlates Motion", 1.5f, 27.0f);

            ITEM_RECT(1.5f, 26.0f, 7.5f, 27.0f) = "timeline/USGS_Earthquake";
            //ITEM_RECT(1.5f, 27.0f, 7.5f, 28.0f) = "timeline/GPlates";

            // Selected item / button
            ITEM_RECT(28.4f, 3.0f, 29.0f, 4.0f) = "button/about?";
            ITEM_RECT(29.4f, 3.0f, 30.0f, 4.0f) = "button/ch_en";
            DRAW_TEXT("En", 29.5f, 3.0f);

            ITEM_RECT(30.5f, 6.0f, 31.5f, 7.9f) = "button/layers";
            ITEM_RECT(30.5f, 8.0f, 31.5f, 9.9f) = "button/locate?";
            ITEM_RECT(30.5f, 10.0f, 31.5f, 11.9f) = "button/show_globe";
            ITEM_RECT(30.5f, 12.0f, 31.5f, 13.9f) = "button/show_ocean";
            ITEM_RECT(30.5f, 14.0f, 31.5f, 15.9f) = "button/home";
            ITEM_RECT(30.5f, 16.0f, 31.5f, 17.9f) = "button/list?";
            ITEM_RECT(30.5f, 18.0f, 31.5f, 19.9f) = "button/one_n?";
            ITEM_RECT(30.5f, 20.0f, 31.5f, 21.9f) = "button/table?";
            ITEM_RECT(30.5f, 22.0f, 31.5f, 23.9f) = "button/coordinate";
            ITEM_RECT(30.5f, 24.0f, 31.5f, 25.9f) = "button/auto_rotate";
            ITEM_RECT(30.5f, 26.0f, 31.5f, 27.9f) = "button/time";

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
                    if (it->second.find("vdb/") != std::string::npos) _volumeMode = 1;
                    else _volumeMode = 0;
                    commandMutex.lock();
                    commandList.insert(it->second);
                    commandMutex.unlock();
                }
                break;
            }
            if (_buttonState > 0) _buttonState = 0;

            // Volume data display
            if (_volumeMode > 0)
            {
                osg::Vec2 pos(wCell * 28.75f, hCell * 14.0f);
                drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 0.5f, hCell * 10.0f),
                                      0.0f, 0.0f, osgVerse::DrawerStyleData(_volume.get()));

                std::ostringstream oss1, oss2;
                oss1 << std::fixed << std::setprecision(2) << global_volumeRange[1]; DRAW_TEXT_MID(oss1.str(), 29.0f, 13.0f);
                oss2 << std::fixed << std::setprecision(2) << global_volumeRange[0]; DRAW_TEXT_MID(oss2.str(), 29.0f, 24.5f);
            }

            if (global_particleIndex >= 0)
            {
                switch (global_particleIndex)
                {
                case 0: DRAW_TEXT("Earthquakes in the First Half of Year 2020", 1.5f, 28.0f); break;
                case 1: DRAW_TEXT("Earthquakes in the Second Half of Year 2020", 1.5f, 28.0f); break;
                case 2: DRAW_TEXT("Earthquakes in the First Half of Year 2021", 1.5f, 28.0f); break;
                case 3: DRAW_TEXT("Earthquakes in the Second Half of Year 2021", 1.5f, 28.0f); break;
                case 4: DRAW_TEXT("Earthquakes in the First Half of Year 2022", 1.5f, 28.0f); break;
                case 5: DRAW_TEXT("Earthquakes in the Second Half of Year 2022", 1.5f, 28.0f); break;
                case 6: DRAW_TEXT("Earthquakes in the First Half of Year 2023", 1.5f, 28.0f); break;
                case 7: DRAW_TEXT("Earthquakes in the Second Half of Year 2023", 1.5f, 28.0f); break;
                case 8: DRAW_TEXT("Earthquakes in the First Half of Year 2024", 1.5f, 28.0f); break;
                case 9: DRAW_TEXT("Earthquakes in the Second Half of Year 2024", 1.5f, 28.0f); break;
                case 10: DRAW_TEXT("Earthquakes in the First Half of Year 2025", 1.5f, 28.0f); break;
                }
            }
            DRAW_TEXT("Earth Plates at: " + std::to_string(-_paleoAgeList[_paleoIndex]) + "Ma", 15.5f, 28.0f);

            // LLA, compass and scale display
            double northRadians = 0.0, widthScale = 0.0;
            if (camera && camera->getViewport())
            {
                osg::Vec3d worldNorth(0.0, 0.0, osg::WGS_84_RADIUS_POLAR);
                osg::Vec3d viewNorth = worldNorth * camera->getViewMatrix();
                osg::Vec2d compassDirection(viewNorth.x(), viewNorth.y());
                compassDirection.normalize();
                northRadians = std::atan2(compassDirection.y(), compassDirection.x());
                DRAW_TEXT_MID(radiansToCompassHeading(northRadians), 29.0f, 9.5f);

                double fovy, aspect, nearPlane, farPlane, distance;
                distance = (osg::Vec3d() * camera->getViewMatrix()).length() - osg::WGS_84_RADIUS_POLAR;
                camera->getProjectionMatrixAsPerspective(fovy, aspect, nearPlane, farPlane);
                double heightNear = 2.0 * distance * tan(osg::inDegrees(fovy) / 2.0);
                widthScale = heightNear / camera->getViewport()->height();
                if (widthScale < 1.0)
                {
                    std::string wStr = std::to_string((int)(widthScale * 100.0) * 0.01f);
                    DRAW_TEXT_MID("S = " + wStr + "m/pixel", 29.0f, 10.5f);
                }
                else
                {
                    int wValue = (int)widthScale;
                    std::string wStr = (wValue > 1000.0) ? std::to_string((int)(wValue * 0.001)) + "km/pixel"
                                                         : std::to_string((int)wValue) + "m/pixel";
                    DRAW_TEXT_MID("S = " + wStr, 29.0f, 10.5f);
                }
            }

            if (manipulator)
            {
                osg::Vec3d lla = manipulator->getLatestPosition();
                DRAW_TEXT(radiansToDMS(lla[1], 'E', 'W'), 25.0f, 28.5f);
                DRAW_TEXT(radiansToDMS(lla[0], 'N', 'S'), 27.5f, 28.5f);
                if (widthScale < 10000) DRAW_TEXT(std::to_string((int)lla[2]) + "m", 30.0f, 28.5f);
            }

            // Compass
            osg::Vec2 pos(wCell * 28.0f, hCell * 6.0f);
            drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 2.0f, hCell * 32.0f / 9.0f),
                                  0.0f, 0.0f, osgVerse::DrawerStyleData(_compass0.get()));
            drawer->translate(-pos - osg::Vec2(wCell, hCell * 16.0f / 9.0f), false);
            drawer->rotate(-northRadians, true);
            drawer->translate(pos + osg::Vec2(wCell, hCell * 16.0f / 9.0f), true);
            drawer->drawRectangle(osg::Vec4(pos[0], pos[1], wCell * 2.0f, hCell * 32.0f / 9.0f),
                                  0.0f, 0.0f, osgVerse::DrawerStyleData(_compass1.get()));
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
    osg::Image* createImageFromPaleo(std::istream& fin, char sep = ',')
    {
        osg::Image* image = new osg::Image;
        image->allocateImage(180, 360, 1, GL_LUMINANCE, GL_FLOAT);
        image->setInternalTextureFormat(GL_LUMINANCE32F_ARB);
        memset(image->data(), 0, image->getTotalSizeInBytes());

        std::string line0; float* ptr = (float*)image->data();
        while (std::getline(fin, line0))
        {
            std::string line = trim(line0);
            if (line.empty()) continue; else if (line[0] == '#') continue;

            std::vector<std::string> values; osgDB::split(line, values, sep);
            if (values.size() > 2)
            {
                float lat = atof(values[1].c_str());
                float lon = atof(values[0].c_str());
                float ati = atof(values[2].c_str());
                int xx = (int)(lat + 90.0f), yy = (int)(lon + 180.0f);
                *(ptr + xx + yy * 180) = ati;
            }
        }
        return image;
    }

    static std::string trim(const std::string& str)
    {
        if (!str.size()) return str;
        std::string::size_type first = str.find_first_not_of(" \t");
        std::string::size_type last = str.find_last_not_of("  \t\r\n");
        if ((first == str.npos) || (last == str.npos)) return std::string("");
        return str.substr(first, last - first + 1);
    }

    std::map<int, std::string> _paleoMaps;
    std::vector<int> _paleoAgeList;
    osg::ref_ptr<PaleoCallback> _paleoCallback;
    osg::ref_ptr<osgVerse::Drawer2D> _drawer;
    osg::ref_ptr<osg::Image> _selected, _volume;
    osg::ref_ptr<osg::Image> _compass0, _compass1;
    int _width, _height, _buttonState, _volumeMode, _paleoIndex;
};

void configureUI(osgViewer::View& viewer, osg::Group* root, const std::string& mainFolder, int w, int h)
{
    osg::ref_ptr<osg::Image> background = osgDB::readImageFile(mainFolder + "/ui/main.png");
    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
    drawer->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    drawer->loadFont("default", mainFolder + "/ui/pingfang.ttf");
    drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));

#if 0
    osg::Camera* hudCamera = osgVerse::createHUDCamera(NULL, w, h, osg::Vec3(), 1.0, 1.0, true);
    hudCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    hudCamera->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    hudCamera->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    hudCamera->setRenderOrder(osg::Camera::POST_RENDER, 20000);
#else
    finalBuffer2 = osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGBA_INT8, w, h);
    finalBuffer2->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
    finalBuffer2->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    finalBuffer2->setWrap(osg::Texture2D::WRAP_S, osg::Texture::CLAMP);
    finalBuffer2->setWrap(osg::Texture2D::WRAP_T, osg::Texture::CLAMP);

    osg::Camera* hudCamera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, NULL, NULL, true);
    hudCamera->setViewport(0, 0, finalBuffer2->getTextureWidth(), finalBuffer2->getTextureHeight());
    hudCamera->attach(osg::Camera::COLOR_BUFFER0, finalBuffer2.get());
#endif
    hudCamera->setClearColor(osg::Vec4(1.0, 0.0, 0.0, 1.0));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(0, osgVerse::createTexture2D(background.get()));
    hudCamera->getOrCreateStateSet()->setTextureAttributeAndModes(1, osgVerse::createTexture2D(drawer.get()));

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
