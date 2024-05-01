#include <osg/io_utils>
#include <osg/LineWidth>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgDB/FileUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgUtil/Simplifier>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>
#include <modeling/Utilities.h>
#include <modeling/Math.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

static std::map<int, osg::observer_ptr<osg::Node>> g_toothMap;
static std::map<int, std::vector<osg::Vec3>> g_borderMap;
static std::map<int, osg::Quat> g_obbMap;
static osg::ref_ptr<osg::Geometry> g_delaunay;

bool createDelaunay(osg::Geometry* geom)
{
    osgVerse::PointList3D points; osgVerse::PointList2D points2D;
    osgVerse::EdgeList edges; osg::BoundingBox bb;
    for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
         itr != g_borderMap.end(); ++itr)
    {
        std::vector<osg::Vec3>& border = itr->second;
        size_t startIdx = 0, endIdx = 0; osg::Vec3 center;
        for (size_t i = 0; i < border.size(); ++i) center += border[i];
        center *= 1.0f / (float)border.size();

        osg::Quat& q = g_obbMap[itr->first];
        for (size_t i = 0; i < border.size(); ++i)
        {
            size_t idx = points.size();
            if (i == border.size() - 1) endIdx = idx;
            else if (i == 0) startIdx = idx;
            points.push_back(border[i]); bb.expandBy(border[i]);

            osg::Vec3 point = center + (border[i] - center);
            points2D.push_back(osgVerse::PointType2D(
                osg::Vec2(point.x(), point.y()), idx));
            if (i > 0) edges.push_back(osgVerse::EdgeType(idx - 1, idx));
        }
        edges.push_back(osgVerse::EdgeType(endIdx, startIdx));
    }

    // Outer boundary
    size_t outerIdx0 = points.size();
    bb._min -= osg::Vec3(1.0f, 1.0f, 0.0f); bb._max += osg::Vec3(1.0f, 1.0f, 0.0f);
    for (size_t i = 0; i < 4; ++i)
    {
        points.push_back(bb.corner(i));
        points2D.push_back(osgVerse::PointType2D(
            osg::Vec2(points.back()[0], points.back()[1]), outerIdx0 + i));
    }
    edges.push_back(osgVerse::EdgeType(outerIdx0 + 0, outerIdx0 + 1));
    edges.push_back(osgVerse::EdgeType(outerIdx0 + 1, outerIdx0 + 3));
    edges.push_back(osgVerse::EdgeType(outerIdx0 + 3, outerIdx0 + 2));
    edges.push_back(osgVerse::EdgeType(outerIdx0 + 2, outerIdx0 + 0));

    // Generate geometry
    osg::DrawElementsUInt* de = static_cast<osg::DrawElementsUInt*>(geom->getPrimitiveSet(0));
    osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
    osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(geom->getColorArray());

    std::vector<size_t> indices =
        osgVerse::GeometryAlgorithm::delaunayTriangulation(points2D, edges);
    if (indices.empty()) return false; else de->assign(indices.begin(), indices.end());

    size_t numVA = points.size();
    if (va->size() != numVA) { va->resize(numVA); ca->resize(numVA); }
    for (size_t i = 0; i < numVA; ++i)
    { (*va)[i] = points[i]; (*ca)[i] = osg::Vec4(0.82f, 0.58f, 0.59f, 1.0f); }

    de->dirty(); va->dirty(); ca->dirty(); geom->dirtyBound();
    //osgUtil::Simplifier simplifier(15.0f); simplifier.simplify(*geom);
    osgUtil::SmoothingVisitor::smooth(*geom); return true;
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Group> root = new osg::Group;
    if (argc < 2) return 0;

    std::string dirName = argv[1]; char end = *dirName.rbegin();
    if (end != '/' && end != '\\') dirName += osgDB::getNativePathSeparator();

    osgDB::DirectoryContents contents = osgDB::getDirectoryContents(dirName);
    for (size_t i = 0; i < contents.size(); ++i)
    {
        std::string fileName = contents[i], line;
        std::string ext = osgDB::getFileExtension(fileName);
        if (ext != "obj") continue;

        int id = std::stoi(fileName); if (id < 30) continue;  // FIXME
        if (fileName.find("border") != std::string::npos)
        {
            std::ifstream in(dirName + fileName);
            std::vector<osg::Vec3>& border = g_borderMap[id];
            while (std::getline(in, line))
            {
                std::stringstream ss; ss << line;
                std::string tag; osg::Vec3 pos;
                ss >> tag >> pos[0] >> pos[1] >> pos[2];
                if (border.size() > 0)
                {
                    osg::Vec3 last = border.back();
                    if (last != pos) border.push_back(pos);
                }
                else border.push_back(pos);
            }
            if (border.back() == border.front()) border.pop_back();
        }
        else
        {
            g_toothMap[id] = osgDB::readNodeFile(dirName + fileName + ".-90,0,0.rot");
            root->addChild(g_toothMap[id].get());

            osgVerse::BoundingVolumeVisitor bvv;
            g_toothMap[id]->accept(bvv); bvv.computeOBB(g_obbMap[id]);
        }
    }

    osg::ref_ptr<osg::Geode> borderNode = new osg::Geode;
    for (std::map<int, std::vector<osg::Vec3>>::iterator itr = g_borderMap.begin();
         itr != g_borderMap.end(); ++itr)
    {
        std::vector<osg::Vec3>& border = itr->second;
        osg::Vec3Array* va = new osg::Vec3Array;
        osg::Vec4Array* ca = new osg::Vec4Array; ca->push_back(osg::Vec4());
        for (size_t i = 0; i < border.size(); ++i) { va->push_back(border[i]); }
        
        osg::Geometry* geom = new osg::Geometry;
        geom->setName(std::to_string(itr->first));
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);
        geom->setVertexArray(va);
        geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_OVERALL);
        geom->addPrimitiveSet(new osg::DrawArrays(GL_LINE_LOOP, 0, va->size()));
        geom->getOrCreateStateSet()->setAttributeAndModes(new osg::LineWidth(4.0f));
        geom->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        borderNode->addDrawable(geom);
    }
    borderNode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    borderNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(borderNode.get());

    osg::ref_ptr<osg::Geode> delaunayNode = new osg::Geode;
    {
        osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);
        osg::Vec3Array* va = new osg::Vec3Array;
        osg::Vec4Array* ca = new osg::Vec4Array;

        g_delaunay = new osg::Geometry;
        g_delaunay->setUseDisplayList(false);
        g_delaunay->setUseVertexBufferObjects(true);
        g_delaunay->setVertexArray(va);
        g_delaunay->setColorArray(ca); g_delaunay->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
        g_delaunay->addPrimitiveSet(de); createDelaunay(g_delaunay.get());
        delaunayNode->addDrawable(g_delaunay.get());
    }
    delaunayNode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    root->addChild(delaunayNode.get());

    // Handler related
    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    std::map<int, osg::observer_ptr<osg::Node>>::iterator teethItr = g_toothMap.end();
    std::map<int, std::vector<osg::Vec3>>::iterator borderItr = g_borderMap.end();

    handler->addKeyUpCallback(osgGA::GUIEventAdapter::KEY_Left, [&](int key) {
        if (teethItr != g_toothMap.begin()) { teethItr--; borderItr--; }
        else { teethItr = g_toothMap.end(); borderItr = g_borderMap.end(); }
        for (size_t i = 0; i < borderNode->getNumDrawables(); ++i)
        {
            osg::Geometry* g = borderNode->getDrawable(i)->asGeometry();
            osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(g->getColorArray());
            if (g->getName() != std::to_string(borderItr->first)) ca->back() = osg::Vec4();
            else ca->back() = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f); ca->dirty();
        }
    });

    handler->addKeyUpCallback(osgGA::GUIEventAdapter::KEY_Right, [&](int key) {
        if (teethItr != g_toothMap.end()) { teethItr++; borderItr++; }
        else { teethItr = g_toothMap.begin(); borderItr = g_borderMap.begin(); }
        for (size_t i = 0; i < borderNode->getNumDrawables(); ++i)
        {
            osg::Geometry* g = borderNode->getDrawable(i)->asGeometry();
            osg::Vec4Array* ca = static_cast<osg::Vec4Array*>(g->getColorArray());
            if (g->getName() != std::to_string(borderItr->first)) ca->back() = osg::Vec4();
            else ca->back() = osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f); ca->dirty();
        }
    });

    handler->addKeyDownCallback('k', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].y() += 0.01f;
        if (!createDelaunay(g_delaunay.get())) return;
        m.setTrans(m.getTrans() + osg::Vec3(0.0f, 0.01f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyDownCallback('i', [&](int key) {
        if (teethItr == g_toothMap.end()) return;
        osg::MatrixTransform* mt = teethItr->second->asTransform()->asMatrixTransform();
        std::vector<osg::Vec3>& border = borderItr->second; osg::Matrix m = mt->getMatrix();
        for (size_t i = 0; i < border.size(); ++i) border[i].y() -= 0.01f;
        if (!createDelaunay(g_delaunay.get())) return;
        m.setTrans(m.getTrans() - osg::Vec3(0.0f, 0.01f, 0.0f)); mt->setMatrix(m);
    });

    handler->addKeyUpCallback('h', [&](int key) {
        for (std::map<int, osg::observer_ptr<osg::Node>>::iterator itr = g_toothMap.begin();
             itr != g_toothMap.end(); ++itr) itr->second->setNodeMask(~itr->second->getNodeMask());
    });

    // Start viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(handler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
