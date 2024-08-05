#include <osg/io_utils>
#include <osg/Multisample>
#include <osg/Texture2D>
#include <osg/PolygonOffset>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

osg::Node* createBlendingArea(osg::Node* input, float offset, const osg::Vec3& dir, const osg::Vec3& up)
{
    osgVerse::MeshCollector mc; mc.setWeldingVertices(true);
    mc.setUseGlobalVertices(true); input->accept(mc);
    const std::vector<osg::Vec3> vertices = mc.getVertices();
    const std::vector<unsigned int> triangles = mc.getTriangles();

    // 1. Find boundary edges, which only have 1 triangle attached
    std::map<osgVerse::EdgeType, int> edgeMap;
    for (size_t i = 0; i < triangles.size(); i += 3)
    {
        unsigned int i0 = triangles[i], i1 = triangles[i + 1], i2 = triangles[i + 2];
        osgVerse::EdgeType e0(osg::minimum(i0, i1), osg::maximum(i0, i1));
        osgVerse::EdgeType e1(osg::minimum(i0, i2), osg::maximum(i0, i2));
        osgVerse::EdgeType e2(osg::minimum(i1, i2), osg::maximum(i1, i2));
        edgeMap[e0]++; edgeMap[e1]++; edgeMap[e2]++;
    }

    std::set<unsigned int> boundary;
    for (std::map<osgVerse::EdgeType, int>::iterator itr = edgeMap.begin();
         itr != edgeMap.end(); ++itr)
    {
        if (itr->second != 1) continue;
        boundary.insert(itr->first.first); boundary.insert(itr->first.second);
    }

    // 2. Project boundary points to 2D plane which matches the best view area
    osgVerse::PointList2D points2, inner2; osgVerse::PointList3D points3;
    for (std::set<unsigned int>::iterator itr = boundary.begin();
         itr != boundary.end(); ++itr) { points3.push_back(vertices[*itr]); }
    osg::Matrix mat3To2 = osgVerse::GeometryAlgorithm::project(points3, -dir, up, points2);
    osgVerse::GeometryAlgorithm::reorderPointsInPlane(points2);

    // 3. Compute inner offset of the boundary points, getting entire edge-list
    osg::Vec2d poi = osgVerse::GeometryAlgorithm::getPoleOfInaccessibility(points2);
    for (size_t i = 0; i < points2.size(); ++i)
    {
        const osgVerse::PointType2D& pt = points2[i];
        osg::Vec2d dir = pt.first - poi; float length = dir.normalize() * offset;
        inner2.push_back(osgVerse::PointType2D(osg::Vec2d(pt.first - dir * length), pt.second));
    }

    osgVerse::EdgeList edges; unsigned int base = points2.size();
    for (size_t i = 0; i < points2.size(); ++i)
        edges.push_back(osgVerse::EdgeType((i > 0) ? (i - 1) : base - 1, i));
    for (size_t i = 0; i < inner2.size(); ++i)
        edges.push_back(osgVerse::EdgeType(base + ((i > 0) ? (i - 1) : inner2.size() - 1), base + i));
    points2.insert(points2.end(), inner2.begin(), inner2.end());

    // 4. Delaunay triangulation to get new geometry data
    osg::Vec3Array* va = new osg::Vec3Array; osg::Vec2Array* ta = new osg::Vec2Array;
    osg::DrawElementsUShort* de = new osg::DrawElementsUShort(GL_TRIANGLES);
    std::vector<size_t> indices = osgVerse::GeometryAlgorithm::delaunayTriangulation(points2, edges);
    if (indices.empty())
    {
        de->setMode(GL_LINES);  // failed, show the boundary to see what happened
        for (size_t i = 0; i < base; ++i)
        {
            const osgVerse::PointType2D& pt = points2[i];
            //va->push_back(osg::Vec3(pt.first, 0.0f)); ta->push_back(osg::Vec2()); de->push_back(i);
            va->push_back(points3[pt.second]); ta->push_back(osg::Vec2()); de->push_back(i);
            if (i > 0) de->push_back(i - 1); else de->push_back(base - 1);
        }
    }
    else
    {
        osg::Matrix mat2To3 = osg::Matrix::inverse(mat3To2);
        for (size_t i = 0; i < points2.size(); ++i)
        {
            const osgVerse::PointType2D& pt = points2[i];
            if (i >= base)
            {
                const osg::Vec2d& v = pt.first; double z = -1000.0;
                osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                    input, osg::Vec3d(v[0], v[1], -z) * mat2To3, osg::Vec3d(v[0], v[1], z) * mat2To3);
                if (!result.drawable.valid())
                {
                    OSG_WARN << "No intersection found for " << v << std::endl;
                    va->push_back(osg::Vec3d(v[0], v[1], 0.0) * mat2To3);
                }
                else
                    va->push_back(result.getWorldIntersectPoint());
                ta->push_back(osg::Vec2(0.0f, 0.0f));
            }
            else
                { va->push_back(points3[pt.second]); ta->push_back(osg::Vec2(1.0f, 0.0f)); }
        }
        de->assign(indices.begin(), indices.end());
    }

    // 5. Output the geometry
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false); geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va); geom->setTexCoordArray(0, ta); geom->addPrimitiveSet(de);
    geom->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(256, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        for (int i = 0; i < 256; ++i)
        { float r = (float)i / 255.0f; image->setColor(osg::Vec4(r, r, r, r), i, 0); }
        tex->setImage(0, image.get());
    }
    geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet()->setAttributeAndModes(new osg::PolygonOffset(-1.1f, 1.0f));
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    geode->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
    geode->addDrawable(geom.get());
    return geode.release();
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::Vec3 direction = osg::Y_AXIS;
    arguments.read("--dir-x", direction.x());
    arguments.read("--dir-y", direction.y());
    arguments.read("--dir-z", direction.z());

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) { OSG_WARN << "Failed to load polygon file" << std::endl; return 1; }

    osg::Vec3 side, up = osg::Z_AXIS; direction.normalize();
    if (direction.z() > 0.99f || direction.z() < -0.99f) up = osg::Y_AXIS;
    side = up ^ direction; up = direction ^ side;

    osg::ref_ptr<osg::Group> root = new osg::Group;
    //root->addChild(scene.get());
    root->addChild(createBlendingArea(scene.get(), 0.1f, direction, up));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
