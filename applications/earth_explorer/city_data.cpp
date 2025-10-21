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
    "uniform float HdrExposure, GlobalOpaque;\n"

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

    "void main() {\n"
    "    vec3 WSD = WorldSunDir, WCP = WorldCameraPos; \n"
    "    vec3 P = vertexInWorld, N = normalInWorld; \n"
    "    vec4 groundColor = vec4(color.rgb, 1.0);\n"

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
        _cityRoot->addChild(createBatchData("Batches/shanghai_buildings"));
        _cityRoot->addChild(createBatchData("Batches/shanghai_vehicles"));
    }

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        osgVerse::EarthManipulator* manipulator =
            static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator());
        if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            //
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == 'z')
            {
                manipulator->setByEye(osgVerse::Coordinate::convertLLAtoECEF(
                    osg::Vec3d(osg::inDegrees(31.35), osg::inDegrees(121.34), 20e4)));
            }
        }
        return false;
    }

protected:
    osg::Group* createBatchData(const std::string& dir)
    {
        osg::Group* batchRoot = new osg::Group;
        osg::ref_ptr<osgDB::Options> opt = new osgDB::Options("Downsamples=10");
        osgDB::DirectoryContents contents = osgDB::getDirectoryContents(_mainFolder + dir);

        for (size_t i = 0; i < contents.size(); ++i)
        {
            const std::string& f = contents[i];
            if (f.empty() || f[0] == '.') continue;
            if (f.find("statistics") != std::string::npos) continue;

            osg::PagedLOD* lod = new osg::PagedLOD;
            lod->addChild(osgDB::readNodeFile(_mainFolder + dir + "/" + f, opt.get()));
            lod->setFileName(1, _mainFolder + dir + "/" + f);

            lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
            lod->setRange(0, 0.0f, 1000.0f);
            lod->setRange(1, 1000.0f, FLT_MAX);
            batchRoot->addChild(lod);
            std::cout << "\rBatch: " << (i + 1) << " / " << contents.size() << "\t\t";
        }
        std::cout << "Loaded " << _mainFolder + dir << "\n";
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
    std::string _mainFolder;
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
    earthRenderingUtils.apply(cityRoot->getOrCreateStateSet(), vs, fs, 1);

    viewer.addEventHandler(new CreateCityHandler(cityRoot.get(), earthRoot->asGroup(), mainFolder));
    return cityRoot.release();
}
