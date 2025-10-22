#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/FileNameUtils>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Tessellator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/Utilities.h>
#include <pipeline/IntersectionManager.h>
#include <readerwriter/EarthManipulator.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>
USE_OSGPLUGIN(verse_csv)

const char* cityVertCode = {
    "uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse; \n"
    "VERSE_VS_OUT vec3 normalInWorld; \n"
    "VERSE_VS_OUT vec3 vertexInWorld; \n"
    "VERSE_VS_OUT vec4 texCoord, color; \n"

    "void main() {\n"
    "    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV; \n"
    "    vertexInWorld = vec3(modelMatrix * osg_Vertex); \n"
    "    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0))); \n"
    "    texCoord = osg_MultiTexCoord0; color = osg_Color; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* cityFragCode = {
    "uniform sampler2D GlareSampler;\n"
    "uniform sampler2D TransmittanceSampler;\n"
    "uniform sampler2D SkyIrradianceSampler;\n"
    "uniform sampler3D InscatterSampler;\n"
    "uniform vec3 WorldCameraPos, WorldSunDir;\n"
    "uniform float HdrExposure, GlobalOpaque, SunAngle, CurrentLon;\n"
    "uniform float osg_SimulationTime, BuildingType;\n"

    "VERSE_FS_IN vec3 normalInWorld; \n"
    "VERSE_FS_IN vec3 vertexInWorld; \n"
    "VERSE_FS_IN vec4 texCoord, color; \n"

    "#ifdef VERSE_GLES3\n"
    "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
    "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
    "#endif\n"

    "#define SUN_INTENSITY 100.0\n"
    "#define PLANET_RADIUS 6360000.0\n"
    "#include \"scattering.module.glsl\"\n"
    "vec3 hdr(vec3 L) {\n"
    "    L = L * HdrExposure; \n"
    "    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r); \n"
    "    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g); \n"
    "    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b); \n"
    "    return L; \n"
    "}\n"

    "float random(vec2 st) { return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123); }\n"
    "float noise(vec2 st) {\n"
    "    vec2 i = floor(st), f = fract(st);\n"
    "    float a = random(i), b = random(i + vec2(1.0, 0.0)); \n"
    "    float c = random(i + vec2(0.0, 1.0)), d = random(i + vec2(1.0, 1.0)); \n"
    "    vec2 u = f * f * (3.0 - 2.0 * f); \n"
    "    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y; \n"
    "}\n"

    "vec3 residentialWindows(vec2 uv, vec2 gridSize, float timeOfDay) { \n"
    "    float frameWidth = 0.06; vec3 lightColor; \n"
    "    vec2 cellUV = uv * gridSize; vec2 cellID = floor(cellUV); vec2 localUV = fract(cellUV); \n"
    "    vec2 frame = step(vec2(frameWidth), localUV) * step(localUV, vec2(1.0 - frameWidth)); \n"
    "    float isWindow = frame.x * frame.y; \n"
    //   Lights of night
    "    float lightProbability = 1.0 - timeOfDay, randomVal = random(cellID * 1.5); \n"
    "    float isLightOn = step(randomVal, lightProbability * 0.4), colorChoice = random(cellID * 2.0); \n"
    //   Colors
    "    if (colorChoice < 0.7) lightColor = vec3(0.9, 0.8, 0.6); \n"
    "    else if (colorChoice < 0.85) lightColor = vec3(0.8, 0.9, 1.0); \n"
    "    else lightColor = vec3(0.9, 0.7, 0.9); \n"
    "    lightColor *= (0.7 + random(cellID + 0.3) * 0.6); \n"
    "    vec3 buildingColor = vec3(0.15, 0.18, 0.22), darkWindowColor = vec3(0.08, 0.1, 0.13); \n"
    "    vec3 color = mix(buildingColor, mix(darkWindowColor, lightColor, isLightOn), isWindow); \n"
    "    if (isLightOn > 0.5 && isWindow > 0.5) {\n"
    "        float glow = (1.0 - length(localUV - 0.5) * 2.0);\n"
    "        color += lightColor * smoothstep(0.0, 0.5, glow) * 0.2;\n"
    "    }\n"
    "    return color;\n"
    "}\n"

    "vec3 modernOfficeWindows(vec2 uv, vec2 gridSize, float timeOfDay) {\n"
    "    float outerFrame = 0.08, innerFrame = 0.03; \n"
    "    vec2 cellUV = uv * gridSize; vec2 cellID = floor(cellUV); vec2 localUV = fract(cellUV); \n"
    "    vec2 outer = step(vec2(outerFrame), localUV) * step(localUV, vec2(1.0 - outerFrame));\n"
    "    float isOuterWindow = outer.x * outer.y; \n"
    "    float crossX = step(innerFrame, localUV.x) * step(localUV.x, 1.0 - innerFrame); \n"
    "    float crossY = step(innerFrame, localUV.y) * step(localUV.y, 1.0 - innerFrame); \n"
    "    float isInnerWindow = crossX * crossY; \n"
    //   Lights of night
    "    float lightProbability = 1.0 - timeOfDay, randomVal = random(cellID); \n"
    "    float isLightOn = step(randomVal, lightProbability * 0.6); \n"
    //   Colors
    "    vec3 buildingColor = vec3(0.1, 0.15, 0.2), darkWindowColor = vec3(0.05, 0.08, 0.12); \n"
    "    vec3 lightWindowColor = vec3(0.9, 0.8, 0.6) * (0.8 + random(cellID + 0.5) * 0.4); \n"
    "    vec3 color = buildingColor; \n"
    "    if (isOuterWindow > 0.5) {\n"
    "        if (isInnerWindow > 0.5) {\n"
    "            color = mix(darkWindowColor, lightWindowColor, isLightOn); \n"
    "            if (isLightOn > 0.5) {\n"
    "                float centerGlow = (1.0 - length(localUV - 0.5) * 1.5); \n"
    "                color += lightWindowColor * smoothstep(0.0, 1.0, centerGlow) * 0.3; \n"
    "            }\n"
    "        }\n"
    "        else color = buildingColor * 0.7;\n"
    "    } \n"
    "    return color; \n"
    "}\n"

    "void main() {\n"
    "    vec3 WSD = WorldSunDir, WCP = WorldCameraPos; \n"
    "    vec3 P = vertexInWorld, N = normalInWorld; \n"
    "    vec4 groundColor = vec4(color.rgb, 1.0);\n"
    "    float nightStart = -1.5707963 + CurrentLon, nightEnd = 1.5707963 + CurrentLon;\n"
    "    float sunAngle = mod(SunAngle, 3.1415926 * 2.0);\n"
    "    float lightIntensity = (sunAngle < nightStart || sunAngle > nightEnd) ? 1.0 : 0.0;\n"

    "    if (BuildingType > 0.5f)\n"
    "        groundColor.rgb *= modernOfficeWindows(texCoord.xy, vec2(15.0, 8.0), lightIntensity);\n"
    "    else if (BuildingType > 0.0f)\n"
    "        groundColor.rgb *= residentialWindows(texCoord.xy, vec2(8.0, 5.0), lightIntensity);\n"
    "    if (BuildingType > 0.0f) groundColor.rgb *= mix(vec3(1.0), vec3(3.0), lightIntensity);\n"

    "    float cTheta = dot(N, WSD); vec3 sunL, skyE; \n"
    "    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE); \n"
    "    groundColor.rgb *= max((sunL * max(cTheta, 0.0) + skyE) / 3.14159265, vec3(0.1)); \n"
    "    groundColor.a *= clamp(GlobalOpaque, 0.0, 1.0); \n"

    "    vec3 extinction = vec3(1.0); \n"
    "    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0); \n"
    "    vec3 compositeColor = groundColor.rgb * extinction + inscatter; \n"
    "    vec4 finalColor = vec4(hdr(compositeColor), groundColor.a); \n"
    "#ifdef VERSE_GLES3\n"
    "    fragColor = finalColor; \n"
    "    fragOrigin = vec4(1.0); \n"
    "#else\n"
    "    gl_FragData[0] = finalColor; \n"
    "    gl_FragData[1] = vec4(1.0); \n"
    "#endif\n"
    "}\n"
};

class CreateCityHandler : public osgGA::GUIEventHandler
{
public:
    CreateCityHandler(osg::Group* root, osg::Group* earth, const std::string& mainFolder)
        : _cityRoot(root), _earthRoot(earth), _mainFolder(mainFolder + "/")
    {
        //_cityRoot->addChild(createBatchData("Batches/shanghai_buildings", false));
        //_cityRoot->addChild(createBatchData("Batches/shanghai_vehicles", true));
        _waitedPagerToFinish = true;
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::USER)
        {
            const osgDB::Options* ev = dynamic_cast<const osgDB::Options*>(ea.getUserData());
            std::string command = ev ? ev->getOptionString() : "";

            std::vector<std::string> commmandPair; osgDB::split(command, commmandPair, '/');
            handleCommand(view, commmandPair.front(), commmandPair.back());
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            if (_loadingCityItem.empty()) return false;
            osg::ref_ptr<osg::Node> city = _cityMap[_loadingCityItem].get();

            if (!city)
            {
                if (!_waitedPagerToFinish || !view->getDatabasePager()->getRequestsInProgress())
                {
                    // FIXME: when to remove city node data?
                    if (_loadingCityItem.find("buildings") != std::string::npos)
                        city = createBatchData("Batches/" + _loadingCityItem, false);
                    else if (_loadingCityItem.find("vehicles") != std::string::npos)
                        city = createBatchData("Batches/" + _loadingCityItem, true);
                    _cityRoot->addChild(city.get()); _cityMap[_loadingCityItem] = city;
                }
            }
            _loadingCityItem = "";
        }
        return false;
    }

    void handleCommand(osgViewer::View* view, const std::string& type, const std::string& cmd)
    {
        osgVerse::EarthManipulator* manipulator =
            static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
        if (type == "item")
        {
            // TODO: def center position?
            manipulator->setByEye(osgVerse::Coordinate::convertLLAtoECEF(
                osg::Vec3d(osg::inDegrees(31.35), osg::inDegrees(121.34), 20e4)));
            _cityRoot->getOrCreateStateSet()->getUniform("CurrentLon")->set((float)osg::inDegrees(121.34));
            _loadingCityItem = cmd;
        }
        else if (type == "value")
        {
            float v = atof(cmd.c_str());
            _cityRoot->getOrCreateStateSet()->getUniform("SunAngle")->set(v);
        }
    }

protected:
    osg::Group* createBatchData(const std::string& dir, bool asVehicles)
    {
        osg::Group* batchRoot = new osg::Group;
        osgDB::DirectoryContents contents = osgDB::getDirectoryContents(_mainFolder + dir);
        osg::ref_ptr<osgDB::Options> opt = new osgDB::Options("Downsamples=10");
        if (_waitedPagerToFinish) opt->setPluginData("EarthRoot", (void*)_earthRoot.get());

        float mid = asVehicles ? 1500.0f : 3500.0f, buildingType = 0.0f; osg::BoundingSphere bsAll;
        for (size_t i = 0; i < contents.size(); ++i)
        {
            const std::string& f = contents[i];
            if (f.empty() || f[0] == '.') continue;
            if (f.find("statistics") != std::string::npos) continue;

            std::string ext = osgDB::getFileExtension(f), roughFile, fineFile;
            if (ext == "csv")
            {
                roughFile = _mainFolder + dir + "_rough/" + f + ".osgb";
                fineFile = _mainFolder + dir + "_fine/" + f + ".osgb";
            }

            osg::ref_ptr<osg::Node> rough, fine; bool roughExists = false;
#if true
            if (!roughFile.empty())
            {
                roughExists = !osgDB::findDataFile(roughFile).empty();
                //rough = osgDB::readNodeFile(roughFile);
            }

            osg::BoundingSphere bsLocal;
            if (!roughExists)
            {
                rough = osgDB::readNodeFile(_mainFolder + dir + "/" + f, opt.get());
                osgDB::makeDirectoryForFile(roughFile); osgDB::writeNodeFile(*rough, roughFile);
                if (!fineFile.empty())
                {
                    fine = osgDB::readNodeFile(_mainFolder + dir + "/" + f);
                    osgDB::makeDirectoryForFile(fineFile); osgDB::writeNodeFile(*fine, fineFile);
                }
                bsLocal = fine.valid() ? fine->getBound() : rough->getBound();
            }
            else
            {
                // FIXME: read from file
                // TODO
            }
#else
            rough = osgDB::readNodeFile(_mainFolder + dir + "/" + f, opt.get()); fineFile = "";
#endif

            osg::PagedLOD* lod = new osg::PagedLOD;
            if (bsLocal.valid())
            {
                lod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                lod->setCenter(bsLocal.center()); lod->setRadius(bsLocal.radius());
                lod->setFileName(0, roughFile);
                bsAll.expandBy(bsLocal);
            }
            else
            {
                if (!rough) rough = osgDB::readNodeFile(roughFile);
                lod->addChild(rough.get());
            }
            lod->setFileName(1, fineFile.empty() ? (_mainFolder + dir + "/" + f) : fineFile);
            lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
            lod->setRange(0, 0.0f, mid); lod->setRange(1, mid, FLT_MAX);

            if (asVehicles) buildingType = -1.0f; else buildingType = (float)rand() / (float)RAND_MAX;
            batchRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("BuildingType", buildingType));
            batchRoot->addChild(lod); std::cout << "\rBatch: " << (i + 1) << " / " << contents.size() << "\t\t";
        }

        osg::Vec3d llhFinal = osgVerse::Coordinate::convertECEFtoLLA(bsAll.center());
        std::cout << "Loaded " << _mainFolder + dir << "\n";
        if (bsAll.valid()) std::cout << "    " << llhFinal << ", Radius = " << bsAll.radius() << "\n";
        return batchRoot;
    }

    osg::Vec3d getViewPosition(osg::Node* city)
    {
        osg::BoundingSphere bs = city->getBound();
        osg::Vec3d lla = osgVerse::Coordinate::convertECEFtoLLA(bs.center());
        lla.z() = 10e4; return osgVerse::Coordinate::convertLLAtoECEF(lla);
    }

    std::map<std::string, osg::observer_ptr<osg::Node>> _cityMap;
    osg::observer_ptr<osg::Group> _cityRoot, _earthRoot;
    std::string _mainFolder, _loadingCityItem;
    bool _waitedPagerToFinish;
};

osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earthRoot,
                             osgVerse::EarthAtmosphereOcean& earthRenderingUtils,
                             const std::string& mainFolder, unsigned int mask)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, cityVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, cityFragCode);
    vs->setName("City_Data_VS"); fs->setName("City_Data_FS");
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);

    osg::ref_ptr<osg::Group> cityRoot = new osg::Group;
    cityRoot->setNodeMask(mask);
    cityRoot->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
    cityRoot->getOrCreateStateSet()->getOrCreateUniform("SceneSampler", osg::Uniform::INT)->set((int)0);
    cityRoot->getOrCreateStateSet()->getOrCreateUniform("SunAngle", osg::Uniform::FLOAT)->set(0.0f);
    cityRoot->getOrCreateStateSet()->getOrCreateUniform("CurrentLon", osg::Uniform::FLOAT)->set(0.0f);
    earthRenderingUtils.apply(cityRoot->getOrCreateStateSet(), vs, fs, 1);

    viewer.addEventHandler(new CreateCityHandler(cityRoot.get(), earthRoot->asGroup(), mainFolder));
    return cityRoot.release();
}
