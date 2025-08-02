#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/Pipeline.h>
#include <pipeline/IntersectionManager.h>
#include <readerwriter/EarthManipulator.h>
#include <VerseCommon.h>
#include <picojson.h>
#include <iostream>
#include <sstream>

const char* vertCode = {
    "uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse; \n"
    "VERSE_VS_OUT vec3 normalInWorld; \n"
    "VERSE_VS_OUT vec3 vertexInWorld; \n"
    "VERSE_VS_OUT vec4 texCoord; \n"

    "void main() {\n"
    "    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV; \n"
    "    vertexInWorld = vec3(modelMatrix * osg_Vertex); \n"
    "    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0))); \n"
    "    texCoord = osg_MultiTexCoord0; gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* fragCode = {
    "uniform sampler2D glareSampler;\n"
    "uniform sampler2D transmittanceSampler;\n"
    "uniform sampler2D skyIrradianceSampler;\n"
    "uniform sampler3D inscatterSampler;\n"
    "uniform vec3 worldCameraPos, worldSunDir, origin;\n"
    "uniform vec3 sunColorScale, skyColorScale;\n"
    "uniform float hdrExposure, globalOpaque;\n"

    "uniform vec3 ColorAttribute;     // (Brightness, Saturation, Contrast)\n"
    "uniform vec3 ColorBalance;       // (Cyan-Red, Magenta-Green, Yellow-Blue)\n"
    "uniform int ColorBalanceMode;    // 0 - Shadow, 1 - Midtone, 2 - Highlight\n"
    "VERSE_FS_IN vec3 normalInWorld; \n"
    "VERSE_FS_IN vec3 vertexInWorld; \n"
    "VERSE_FS_IN vec4 texCoord; \n"

    "#ifdef VERSE_GLES3\n"
    "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
    "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
    "#endif\n"

    "#define SUN_INTENSITY 100.0\n"
    "#define PLANET_RADIUS 6360000.0\n"
    "#include \"scattering.module.glsl\"\n"
    "vec3 hdr(vec3 L) {\n"
    "    L = L * hdrExposure; \n"
    "    L.r = L.r < 1.413 ? pow(L.r * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.r); \n"
    "    L.g = L.g < 1.413 ? pow(L.g * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.g); \n"
    "    L.b = L.b < 1.413 ? pow(L.b * 0.38317, 1.0 / 2.2) : 1.0 - exp(-L.b); \n"
    "    return L; \n"
    "}\n"

    "void main() {\n"
    "    vec3 WSD = worldSunDir, WCP = worldCameraPos; \n"
    "    vec3 P = vertexInWorld, N = normalInWorld; \n"
    "    vec4 groundColor = vec4(1.0);\n"

    "    float cTheta = dot(N, WSD); vec3 sunL, skyE; \n"
    "    sunRadianceAndSkyIrradiance(P, N, WSD, sunL, skyE); \n"
    "    groundColor.rgb *= max((sunL * max(cTheta, 0.0) + skyE) / 3.14159265, vec3(0.1)); \n"
    "    groundColor.a *= clamp(globalOpaque, 0.0, 1.0); \n"

    "    vec3 extinction = vec3(1.0); \n"
    "    vec3 inscatter = inScattering(WCP, P, WSD, extinction, 0.0); \n"
    "    vec3 compositeColor = groundColor.rgb * extinction * sunColorScale + inscatter * skyColorScale; \n"
    "    vec4 finalColor = vec4(hdr(compositeColor), groundColor.a); \n"

    "    finalColor.rgb = colorBalanceFunc(finalColor.rgb, ColorBalance.x, ColorBalance.y, ColorBalance.z, ColorBalanceMode); \n"
    "    finalColor.rgb = colorAdjustmentFunc(finalColor.rgb, ColorAttribute.x, ColorAttribute.y, ColorAttribute.z); \n"
    "#ifdef VERSE_GLES3\n"
    "    fragColor = finalColor; \n"
    "    fragOrigin = vec4(1.0); \n"
    "#else\n"
    "    gl_FragData[0] = finalColor; \n"
    "    gl_FragData[1] = vec4(1.0); \n"
    "#endif\n"
    "}\n"
};

osg::Node* createCity(osg::Node* earthRoot, const std::string& jsonFile)
{
    std::ifstream fin(jsonFile, std::ios::in | std::ios::binary);
    std::string buffer((std::istreambuf_iterator<char>(fin)),
                       std::istreambuf_iterator<char>());
    if (buffer.empty()) return new osg::Node;

    picojson::value root; std::string err = picojson::parse(root, buffer);
    if (!err.empty()) { OSG_WARN << err << "\n"; return new osg::Node; }

    osg::ref_ptr<osg::Group> city = new osg::Group;
    if (root.is<picojson::array>())
    {
        try
        {
            std::vector<osgVerse::GeometryMerger::GeometryPair> geomList;
            std::vector<osg::ref_ptr<osg::Geometry>> geomRefList;
            picojson::array dataList = root.get<picojson::array>();
            for (size_t i = 0; i < dataList.size(); ++i)
            {
                picojson::array centerNode = dataList[i].get("center").get<picojson::array>();
                picojson::array polygonNode = dataList[i].get("polygon").get<picojson::array>();
                std::string color = dataList[i].get("color").get<std::string>();
                double height = dataList[i].get("height").get<double>();

                std::vector<osg::Vec3d> polygon; osg::Vec3d center;
                for (size_t j = 0; j < polygonNode.size(); ++j)
                {
                    picojson::array ptNode = polygonNode[j].get<picojson::array>();
                    if (ptNode.size() > 1) polygon.push_back(
                        osg::Vec3d(ptNode[0].get<double>(), ptNode[1].get<double>(), 0.0));
                }
                if (polygon.size() > 2) polygon.push_back(polygon.front());
                if (centerNode.size() > 1) center.set(  // lat lon
                    osg::inDegrees(centerNode[1].get<double>()), osg::inDegrees(centerNode[0].get<double>()), 500.0);

                osg::Matrix localToWorld = osgVerse::Coordinate::convertLLAtoENU(center);
                osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(center);
                osg::Vec3d N = ecef; N.normalize();

                osgVerse::IntersectionResult result =
                    osgVerse::findNearestIntersection(earthRoot, ecef + N * 1000.0, ecef - N * 1000.0);
                if (result.drawable.valid()) localToWorld.setTrans(result.getWorldIntersectPoint());
                //else OSG_NOTICE << "No intersection for building " << i << ", at file " << jsonFile << "\n";

                std::vector<osgVerse::PointList3D> inner;
                osg::Geometry* geom = osgVerse::createExtrusionGeometry(polygon, inner, osg::Z_AXIS * height);
                geomList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
                geomRefList.push_back(geom); osgUtil::SmoothingVisitor::smooth(*geom);
            }

            osg::Matrix l2w = geomList[0].second; osg::Matrix w2l = osg::Matrix::inverse(l2w);
            for (size_t i = 0; i < geomList.size(); ++i) geomList[i].second = geomList[i].second * w2l;

            osgVerse::GeometryMerger merger;
            osg::ref_ptr<osg::Geometry> mergedGeom = merger.process(geomList, 0);
            osg::Geode* geode = new osg::Geode; geode->addDrawable(mergedGeom.get());

            osg::MatrixTransform* mt = new osg::MatrixTransform;
            mt->setMatrix(l2w); mt->addChild(geode);
            city->addChild(mt); //osgDB::writeNodeFile(*geode, "../beijing.osgb");
        }
        catch (std::exception& e)
        { OSG_WARN << "Failed with json: " << e.what() << "\n"; }
    }
    return city.release();
}

osg::Node* configureCityData(osgViewer::View& viewer, osg::Node* earthRoot,
                             const std::string& mainFolder, unsigned int mask)
{
    osg::ref_ptr<osg::Group> cityRoot = new osg::Group;
    cityRoot->setNodeMask(mask);
    cityRoot->addChild(createCity(earthRoot, mainFolder + "/cities/beijing.json"));

    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, vertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, fragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("City_VS"); program->addShader(vs);
    fs->setName("City_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME
    cityRoot->getOrCreateStateSet()->setAttributeAndModes(program.get());
    return cityRoot.release();
}
