#include <osg/io_utils>
#include <osg/LOD>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Tessellator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <3rdparty/rapidxml/rapidxml.hpp>
#include <3rdparty/rapidxml/rapidxml_utils.hpp>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/Pipeline.h>
#include <pipeline/LightModule.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <readerwriter/EarthManipulator.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/TileCallback.h>
#include <VerseCommon.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#define CONVERT_CSV_DATA 0
#define TEST_CSV_DATA 0

static std::string trim(const std::string& str)
{
    if (!str.size()) return str;
    std::string::size_type first = str.find_first_not_of(" \t");
    std::string::size_type last = str.find_last_not_of("  \t\r\n");
    if ((first == str.npos) || (last == str.npos)) return std::string("");
    return str.substr(first, last - first + 1);
}

static void splitString(const std::string& src, std::vector<std::string>& slist, char sep, bool ignoreEmpty)
{
    if (src.empty()) return;
    std::string::size_type start = 0;
    bool inQuotes = false;

    for (std::string::size_type i = 0; i < src.size(); ++i)
    {
        if (src[i] == '"')
            inQuotes = !inQuotes;
        else if (src[i] == sep && !inQuotes)
        {
            if (!ignoreEmpty || (i - start) > 0)
                slist.push_back(src.substr(start, i - start));
            start = i + 1;
        }
    }
    if (!ignoreEmpty || (src.size() - start) > 0)
        slist.push_back(src.substr(start, src.size() - start));
}

static void tessellateGeometry(osg::Geometry& geom, const osg::Vec3& axis)
{
    osg::ref_ptr<osgUtil::Tessellator> tscx = new osgUtil::Tessellator;
    tscx->setWindingType(osgUtil::Tessellator::TESS_WINDING_ODD);
    tscx->setTessellationType(osgUtil::Tessellator::TESS_TYPE_POLYGONS);
    if (axis.length2() > 0.1f) tscx->setTessellationNormal(axis);
    tscx->retessellatePolygons(geom);
}

static osg::Geometry* createExtrusionGeometry(const osgVerse::PointList3D& outer, const osg::Vec3& height)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array, vaCap = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec3Array> na = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec2Array> ta = new osg::Vec2Array;
    osgVerse::PointList3D pathEx = outer;

    osg::ref_ptr<osg::DrawElementsUInt> deWall = new osg::DrawElementsUInt(GL_QUADS);
    bool closed = (pathEx.front() == pathEx.back());
    if (closed && pathEx.front() == pathEx.back()) pathEx.pop_back();

    size_t eSize = pathEx.size(); float eStep = 1.0f / (float)eSize;
    for (size_t i = 0; i <= eSize; ++i)
    {   // outer walls
        if (!closed && i == eSize) continue; size_t start = va->size();
        va->push_back(pathEx[i % eSize]); ta->push_back(osg::Vec2((float)i * eStep, 0.0f));
        va->push_back(pathEx[i % eSize] + height); ta->push_back(osg::Vec2((float)i * eStep, 1.0f));
        vaCap->push_back(va->back());
        va->push_back(pathEx[(i + 1) % eSize]); ta->push_back(osg::Vec2((float)(i + 1) * eStep, 0.0f));
        va->push_back(pathEx[(i + 1) % eSize] + height); ta->push_back(osg::Vec2((float)(i + 1) * eStep, 1.0f));
        vaCap->push_back(va->back());

        osg::Plane plane(va->at(start), va->at(start + 1), va->at(start + 2));
        osg::Vec3 N = plane.getNormal(); na->push_back(N); na->push_back(N); na->push_back(N); na->push_back(N);
        deWall->push_back(start + 1); deWall->push_back(start);
        deWall->push_back(start + 2); deWall->push_back(start + 3);
    }

    size_t startCap = va->size(); osg::Vec3 hNormal = height; hNormal.normalize();
    osg::ref_ptr<osg::DrawElementsUShort> deCap = new osg::DrawElementsUShort(GL_POLYGON);
    va->insert(va->end(), vaCap->begin(), vaCap->end());
    for (size_t i = startCap; i < va->size(); ++i)
    {
        ta->push_back(osg::Vec2(0.5f, 0.5f));
        na->push_back(hNormal); deCap->push_back(i);
    }

    osg::ref_ptr<osg::Geometry> geom = osgVerse::createGeometry(va.get(), na.get(), ta.get(), deWall.get());
    geom->addPrimitiveSet(deCap.get()); tessellateGeometry(*geom, hNormal); return geom.release();
}

static osg::Vec4 randomColor(unsigned int id)
{
    switch (id % 8)
    {
    case 1: return osg::Vec4(1.0f, 0.5f, 0.5f, 1.0f);
    case 2: return osg::Vec4(0.5f, 1.0f, 0.5f, 1.0f);
    case 3: return osg::Vec4(0.5f, 0.5f, 1.0f, 1.0f);
    case 4: return osg::Vec4(0.5f, 1.0f, 1.0f, 1.0f);
    case 5: return osg::Vec4(1.0f, 1.0f, 0.5f, 1.0f);
    case 6: return osg::Vec4(1.0f, 0.5f, 1.0f, 1.0f);
    case 7: return osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
    default: return osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f);
    }
}

osg::Node* createCityData(const std::string& csvFile)
{
    std::map<size_t, std::string> indexMap;
    std::map<std::string, std::string> valueMap;
    std::string line0; unsigned int rowID = 0;
    std::ifstream in(csvFile.c_str()); double z = 0.0;

    std::vector<osgVerse::GeometryMerger::GeometryPair> geomList;
    std::vector<osgVerse::GeometryMerger::GeometryPair> geomList0;
    std::vector<osg::ref_ptr<osg::Geometry>> geomRefList;
    while (std::getline(in, line0))
    {
        std::string line = trim(line0); rowID++;
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::vector<std::string> values, rings;
        splitString(line, values, ',', false);
        if (!valueMap.empty())
        {
            size_t numColumns = valueMap.size();
            if (numColumns != values.size())
            {
                std::cout << line << "\n";
                std::cout << "CSV line " << rowID << " has different values (" << values.size()
                          << ") than " << numColumns << " header columns" << std::endl; continue;
            }

            for (size_t i = 0; i < values.size(); ++i) valueMap[indexMap[i]] = values[i];
            const std::string& vData = valueMap["vertices"]; double height = atof(valueMap["Z"].c_str());
            std::vector<osg::Vec3d> polygon; osg::Vec3d center; splitString(vData, rings, '|', true);

            std::vector<std::string> vertices; splitString(rings[0], vertices, ' ', true);  // FIXME: only consider outer ring?
            for (size_t j = 0; j < vertices.size(); j += 2)
            {
                polygon.push_back(osg::Vec3d(osg::inDegrees(atof(vertices[j + 1].c_str())),
                                             osg::inDegrees(atof(vertices[j + 0].c_str())), z));
                center += polygon.back();
            }
            center *= 1.0 / (double)polygon.size();
            if (polygon.size() > 2) polygon.push_back(polygon.front());
            
            osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(center);
            osg::Matrix localToWorld = osgVerse::Coordinate::convertLLAtoENU(center);
            osg::Matrix worldToLocal = osg::Matrix::inverse(localToWorld);
            //osg::Matrix localToWorld = osg::Matrix::translate(center);
            osg::Vec3d N = ecef; N.normalize();
            for (size_t j = 0; j < polygon.size(); ++j)
            {
                osg::Vec3d pt = osgVerse::Coordinate::convertLLAtoECEF(polygon[j]);
                polygon[j] = pt * worldToLocal;
            }

            osg::Geometry* geom = createExtrusionGeometry(polygon, osg::Z_AXIS * (height * 3.0));
            osg::Vec4Array* ca = new osg::Vec4Array; geom->setColorArray(ca);
            geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            ca->assign(static_cast<osg::Vec3Array*>(geom->getVertexArray())->size(), randomColor(rowID));

            geomList.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
            geomList0.push_back(osgVerse::GeometryMerger::GeometryPair(geom, localToWorld));
            geomRefList.push_back(geom);
        }
        else
        {
            for (size_t i = 0; i < values.size(); ++i)
                { indexMap[i] = values[i]; valueMap[values[i]] = ""; }
        }
    }

    if (geomList.empty()) { std::cout << "No data?\n"; return NULL; }
    osg::Matrix l2w = geomList[0].second; osg::Matrix w2l = osg::Matrix::inverse(l2w);
    for (size_t i = 0; i < geomList.size(); ++i) geomList[i].second = geomList[i].second * w2l;
    for (size_t i = 0; i < geomList0.size(); ++i) geomList0[i].second = geomList0[i].second * w2l;

    size_t new_sz = 0;  // downsampling?
    for (size_t i = 0; i < geomList.size(); ++i)
    {
        if (i % 5 == 0) continue;
        geomList[new_sz++] = std::move(geomList[i]);
    }
    geomList.resize(new_sz);

    new_sz = 0;
    for (size_t i = 0; i < geomList0.size(); i += 5)
        geomList0[new_sz++] = std::move(geomList0[i]);
    geomList0.resize(new_sz);

    osg::MatrixTransform* mt = new osg::MatrixTransform;
    {
        osgVerse::GeometryMerger merger; osg::Geode* geode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> mergedGeom = merger.process(geomList, 0);
        geode->addDrawable(mergedGeom.get());

        osgVerse::GeometryMerger merger0; osg::Geode* geode0 = new osg::Geode;
        osg::ref_ptr<osg::Geometry> mergedGeom0 = merger0.process(geomList0, 0);
        geode0->addDrawable(mergedGeom0.get());

        osg::LOD* lod = new osg::LOD;
        lod->setRangeMode(osg::LOD::PIXEL_SIZE_ON_SCREEN);
        lod->addChild(geode0, 0.0f, 5000.0f);
        lod->addChild(geode, 5000.0f, FLT_MAX);
        mt->addChild(lod);
    }
    mt->setMatrix(l2w); return mt;
}

osg::Node* createVehicleData(const std::string& csvFile, std::vector<osg::Vec3d>& pts)
{
    std::map<size_t, std::string> indexMap;
    std::map<std::string, std::string> valueMap;
    std::string line0, header; unsigned int rowID = 0;
    std::ifstream in(csvFile.c_str()); double z = 100.0;

    osg::Vec3Array* va = new osg::Vec3Array;
    osg::Vec4Array* ca = new osg::Vec4Array;
    osg::Matrix l2w, w2l; bool matrixSet = false;
    while (std::getline(in, line0))
    {
        std::string line = trim(line0); rowID++;
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        std::vector<std::string> values, vertices;
        splitString(line, values, ',', false);
        if (!valueMap.empty())
        {
            size_t numColumns = valueMap.size();
            if (numColumns != values.size())
            {
                std::cout << header << "\n" << line << "\n";
                std::cout << "CSV line " << rowID << " has different values (" << values.size()
                          << ") than " << numColumns << " header columns" << std::endl; continue;
            }

            for (size_t i = 0; i < values.size(); ++i) valueMap[indexMap[i]] = values[i];
            const std::string& vData = valueMap["vertices"]; double label = atof(valueMap["Label"].c_str());
            std::vector<osg::Vec3d> polygon; osg::Vec3d center; splitString(vData, vertices, ' ', true);
            for (size_t j = 0; j < vertices.size(); j += 2)
            {
                polygon.push_back(osg::Vec3d(osg::inDegrees(atof(vertices[j + 1].c_str())),
                    osg::inDegrees(atof(vertices[j + 0].c_str())), z));
                center += polygon.back();
            }
            center *= 1.0 / (double)polygon.size();
            if (polygon.size() > 2) polygon.push_back(polygon.front());

            osg::Vec3d ecef = osgVerse::Coordinate::convertLLAtoECEF(center);
            osg::Vec3d N = ecef; N.normalize();
            if (!matrixSet)
            {
                l2w = osgVerse::Coordinate::convertLLAtoENU(center);
                w2l = osg::Matrix::inverse(l2w); matrixSet = true;
            }

            va->push_back(ecef * w2l); pts.push_back(ecef);
            if (label > 0.5f) ca->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
            else ca->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
        }
        else
        {
            header = line;
            for (size_t i = 0; i < values.size(); ++i)
                { indexMap[i] = values[i]; valueMap[values[i]] = ""; }
        }
    }

    osg::Geometry* geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va); geom->setColorArray(ca);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, va->size()));

    osg::Geode* geode = new osg::Geode;
    geode->getOrCreateStateSet()->setMode(GL_PROGRAM_POINT_SIZE, osg::StateAttribute::ON);
    geode->addDrawable(geom);

    osg::MatrixTransform* mt = new osg::MatrixTransform;
    mt->setMatrix(l2w); mt->addChild(geode); return mt;
}

osg::StateSet* createPbrStateSet(osgVerse::Pipeline* pipeline)
{
    osg::ref_ptr<osg::StateSet> forwardSS = pipeline->createForwardStateSet(
        osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "std_forward_render.vert.glsl"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "std_forward_render.frag.glsl"));

    osgVerse::LightModule* lm = static_cast<osgVerse::LightModule*>(pipeline->getModule("Light"));
    if (forwardSS.valid() && lm)
    {
        forwardSS->setTextureAttributeAndModes(7, lm->getParameterTable());
        forwardSS->addUniform(new osg::Uniform("LightParameterMap", 7));
        forwardSS->addUniform(lm->getLightNumber());
    }
    return forwardSS.release();
}

class ProcessThread : public OpenThreads::Thread
{
public:
    ProcessThread() : _count(0), _count2(0), _done(false) {}
    std::vector<osg::Vec3d>& getPoints() { return _allPoints; }
    bool done() const { return _done; }

    void set(const osg::Matrix& viewMatrix, const osg::Matrix& projMatrix)
    {
        osg::Polytope frustum; frustum.setToUnitFrustum(false, false);
        frustum.transformProvidingInverse(viewMatrix * projMatrix);
        _mutex.lock(); _frustum = frustum; _mutex.unlock();
    }

    unsigned int getCount()
    {
        if (_mutex2.trylock() == 0) { _count2 = _count; _mutex2.unlock(); }
        return _count2;
    }

    virtual void run()
    {
        while (!_done)
        {
            _mutex2.lock(); _count = 0;
            for (size_t i = 0; i < _allPoints.size(); i += 5000)
            {
                osg::Polytope frustum; _mutex.lock(); frustum = _frustum; _mutex.unlock();
                for (size_t j = 0; j < 5000; ++j)
                {
                    const osg::Vec3d& pt = _allPoints[i + j];
                    if (frustum.contains(pt)) _count++;
                }
                OpenThreads::Thread::microSleep(50);
            }
            _mutex2.unlock(); OpenThreads::Thread::microSleep(150000);
        }
        _done = true;
    }

    virtual int cancel()
    { _done = true; return OpenThreads::Thread::cancel(); }

protected:
    std::vector<osg::Vec3d> _allPoints;
    osg::Polytope _frustum;
    OpenThreads::Mutex _mutex, _mutex2;
    unsigned int _count, _count2;
    bool _done;
};

class EnvironmentHandler : public osgGA::GUIEventHandler
{
public:
    EnvironmentHandler(osgVerse::HeadUpDisplayCanvas& hud, ProcessThread& th)
        : _hud(&hud), _thread(&th) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::KEYUP)
        {
            if (ea.getKey() == 'z')
            {
                static_cast<osgVerse::EarthManipulator*>(view->getCameraManipulator())->setByEye(
                    osgVerse::Coordinate::convertLLAtoECEF(osg::Vec3d(osg::inDegrees(31.35), osg::inDegrees(121.34), 20e4)));
            }
        }
        else if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
        {
            _thread->set(view->getCamera()->getViewMatrix(), view->getCamera()->getProjectionMatrix());
            std::wstring count = std::to_wstring(_thread->getCount());
            _hud->texts["count"]->setText((L"Count = " + count).c_str());
        }
        return false;
    }

protected:
    osgVerse::HeadUpDisplayCanvas* _hud;
    ProcessThread* _thread;
};

static std::string tkCode;
static std::string createCustomPath(int type, const std::string& prefix, int x, int y, int z)
{
    if (type == osgVerse::TileCallback::ORTHOPHOTO)
    {
        if (z > 4)
        {
            std::string prefix2 = "http://t0.tianditu.gov.cn/img_w/wmts?tk=" + tkCode + "&SERVICE=WMTS&REQUEST=GetTile" +
                                  "&VERSION=1.0.0&LAYER=img&STYLE=default&TILEMATRIXSET=w&FORMAT=tiles" +
                                  "&TILEMATRIX={z}&TILEROW={y}&TILECOL={x}";
            return osgVerse::TileCallback::createPath(prefix2, x, pow(2, z) - y - 1, z);
        }
    }
    return osgVerse::TileCallback::createPath(prefix, x, y, z);
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgDB::Registry::instance()->addFileExtensionAlias("tif", "verse_tiff");
    osgVerse::updateOsgBinaryWrappers();

    std::string mainFolder = BASE_DIR + "/models/Earth"; arguments.read("--folder", mainFolder);
    std::string earthURLs = " Orthophoto=mbtiles://" + mainFolder + "/DOM_lv4.mbtiles/{z}-{x}-{y}.jpg"
                            " Elevation=mbtiles://" + mainFolder + "/DEM_lv3.mbtiles/{z}-{x}-{y}.tif"
                            " UseWebMercator=1 OriginBottomLeft=1 TileSkirtRatio=0.05 UseEarth3D=1";
    osg::ref_ptr<osgDB::Options> earthOptions = new osgDB::Options(earthURLs);
    if (arguments.read("--tk", tkCode))
    {
        earthOptions->setPluginStringData("RequestHeaders", "Host;t0.tianditu.gov.cn;Referer;http://www.tianditu.gov.cn");
        earthOptions->setPluginData("UrlPathFunction", (void*)createCustomPath);
        //osgVerse::TileManager::instance()->setTileLoadingOptions(
        //    new osgDB::Options("RequestHeaders=Host;t0.tianditu.gov.cn;Referer;http://www.tianditu.gov.cn"));
    }

    osg::ref_ptr<osg::Node> earth = osgDB::readNodeFile("0-0-0.verse_tms", earthOptions.get());
    if (!earth) return 1;

    osg::ref_ptr<osg::MatrixTransform> group = new osg::MatrixTransform;
    ProcessThread thread;
    {
        // City
#if TEST_CSV_DATA
        size_t i = 1;
#else
        for (size_t i = 1; i < 18; ++i)
#endif
        {
            std::ostringstream oss; oss << std::setw(4) << std::setfill('0') << i;
#if CONVERT_CSV_DATA
            osg::ref_ptr<osg::Node> node = createCityData("E:/zhijiang/shanghai_city/batch_" + oss.str() + ".csv");
            osgDB::writeNodeFile(*node, "E:/zhijiang/shanghai_city/batch_" + oss.str() + ".osgb");
#elif TEST_CSV_DATA
            group->addChild(createCityData("E:/zhijiang/shanghai_city/batch_" + oss.str() + ".csv"));
#else
            group->addChild(osgDB::readNodeFile("E:/zhijiang/shanghai_city/batch_" + oss.str() + ".osgb"));
#endif
            std::cout << "Finished city batch " << i << std::endl;
        }

        // Vehicle
        std::vector<osg::Vec3d>& allPoints = thread.getPoints();
        for (size_t i = 1; i < 22; ++i)
        {
            std::ostringstream oss; oss << std::setw(4) << std::setfill('0') << i;
            group->addChild(createVehicleData("E:/zhijiang/shanghai_vehicle_14/batch_" + oss.str() + ".csv", allPoints));
            std::cout << "Finished vehicle batch " << i << std::endl;
        }
    }

    osg::ref_ptr<osg::Group> root = new osg::Group;
    group->addChild(earth.get());
    root->addChild(group.get());

    osgVerse::HeadUpDisplayCanvas hud;
    root->addChild(hud.create(1920, 1080));
    hud.createText("count", L"Count = 0", 30.0f, 200, 40, "root", osgVerse::HeadUpDisplayCanvas::ROW,
                   osgVerse::HeadUpDisplayCanvas::LEFT | osgVerse::HeadUpDisplayCanvas::TOP, MISC_DIR + "LXGWFasmartGothic.ttf");

    osg::ref_ptr<osgVerse::LightDrawable> light0 = new osgVerse::LightDrawable;
    light0->setColor(osg::Vec3(2.0f, 2.0f, 2.0f));
    light0->setDirection(osg::Vec3(0.02f, 0.1f, -1.0f));
    light0->setDirectional(true);

    osg::ref_ptr<osgVerse::LightDrawable> light1 = new osgVerse::LightDrawable;
    light1->setColor(osg::Vec3(1.0f, 1.0f, 1.0f));
    light1->setDirection(osg::Vec3(-0.4f, 0.6f, 0.1f));
    light1->setDirectional(true);

    osg::ref_ptr<osg::Geode> lightGeode = new osg::Geode;
    lightGeode->addDrawable(light0.get());  // Main light
    lightGeode->addDrawable(light1.get());
    root->addChild(lightGeode.get());

    // The pipeline only for shader construction and lighting
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    osg::ref_ptr<osgVerse::LightModule> lightModule = new osgVerse::LightModule("Light", pipeline.get());
    lightModule->setMainLight(light0.get(), "");  // no shadow module
    group->setStateSet(createPbrStateSet(pipeline.get()));

    osg::ref_ptr<osgVerse::EarthManipulator> manipulator = new osgVerse::EarthManipulator;
    manipulator->setWorldNode(earth.get());

    osgViewer::Viewer viewer;
    osg::ref_ptr<osgVerse::EarthProjectionMatrixCallback> epmcb =
        new osgVerse::EarthProjectionMatrixCallback(viewer.getCamera(), earth->getBound().center());
    epmcb->setNearFirstModeThreshold(2000.0);
    viewer.getCamera()->setClampProjectionMatrixCallback(epmcb.get());
    //viewer.getCamera()->setNearFarRatio(0.00001);

    viewer.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0f));
    viewer.getCamera()->addUpdateCallback(lightModule.get());
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new EnvironmentHandler(hud, thread));
    viewer.setCameraManipulator(manipulator.get());
    viewer.setSceneData(root.get());

    thread.startThread();
    viewer.run();
    thread.cancel(); thread.join();
    return 0;
}
