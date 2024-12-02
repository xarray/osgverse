#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>
#include <pipeline/IntersectionManager.h>
#include <modeling/Utilities.h>
#include <modeling/Math.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> nodeA = (argc < 2)
                                  ? osgDB::readNodeFile("cow.osg") : osgDB::readNodeFile(argv[1]);
    osg::Polytope polytope;
    {
        osg::ComputeBoundsVisitor cbv;
        nodeA->accept(cbv);

        osg::BoundingBox bb = cbv.getBoundingBox();
        osg::Vec3 minPt(bb._min.x() * 0.8f + bb._max.x() * 0.2f, bb._min.y(), bb._min.z());
        osg::Vec3 maxPt(bb._min.x() * 0.5f + bb._max.x() * 0.5f, bb._max.y(), bb._max.z());
        polytope.setToBoundingBox(osg::BoundingBox(minPt, maxPt));
    }

    std::vector<osgVerse::IntersectionResult> results =
        osgVerse::findAllIntersections(nodeA.get(), polytope);
    osg::Vec3Array* va = new osg::Vec3Array;
    osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);

    std::map<int, std::vector<std::pair<osg::Vec3, osg::Vec3>>> edgeOnPlaneMap;
    const osg::Polytope::PlaneList& planes = polytope.getPlaneList();
    for (size_t k = 0; k < results.size(); ++k)
    {
        osgVerse::IntersectionResult& ir = results[k];
        std::vector<osg::Vec3d> origPt = ir.intersectPoints;
        if (origPt.size() < 3) continue;

        // Find and reorder vertices on a intersected face (with 3-5 points)
        std::vector<osg::Vec3d> ptIn, ptOut, edge;
        for (size_t i = 0; i < origPt.size(); ++i) ptIn.push_back(origPt[i] * ir.matrix);

        osgVerse::PointList2D projected; ptOut.resize(ptIn.size());
        osgVerse::GeometryAlgorithm::project(ptIn, osg::Vec3d(), osg::Vec3d(), projected);
        osgVerse::GeometryAlgorithm::reorderPointsInPlane(projected);
        for (size_t i = 0; i < ptIn.size(); ++i) ptOut[i] = ptIn[projected[i].second];

        // Find which plane this face is intersected and record a co-plane edge if exists
        for (size_t j = 0; j < planes.size(); ++j)
        {
            const osg::Plane& plane = planes[j]; edge.clear();
            for (size_t i = 0; i < ptOut.size(); ++i)
            { if (osg::equivalent(plane.distance(ptOut[i]), 0.0)) edge.push_back(ptOut[i]); }
            if (edge.size() > 1)
                edgeOnPlaneMap[j].push_back(std::pair<osg::Vec3, osg::Vec3>(edge[0], edge[1]));
        }

        // Triangulate the face in a simple way
        size_t numPt = ptOut.size(), i0 = va->size();
        for (size_t p = 0; p < numPt; ++p) va->push_back(ptOut[p]);
        switch (numPt)
        {
        case 3: de->push_back(i0); de->push_back(i0 + 1); de->push_back(i0 + 2); break;
        case 4: de->push_back(i0); de->push_back(i0 + 1); de->push_back(i0 + 2);
                de->push_back(i0); de->push_back(i0 + 2); de->push_back(i0 + 3); break;
        case 5: de->push_back(i0); de->push_back(i0 + 1); de->push_back(i0 + 2);
                de->push_back(i0); de->push_back(i0 + 2); de->push_back(i0 + 3);
                de->push_back(i0); de->push_back(i0 + 3); de->push_back(i0 + 4); break;
        }
    }

    // Find each cap and do Delaunay
    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    for (std::map<int, std::vector<std::pair<osg::Vec3, osg::Vec3>>>::iterator
         itr = edgeOnPlaneMap.begin(); itr != edgeOnPlaneMap.end(); ++itr)
    {
        std::vector<osgVerse::LineType3D> edges(itr->second.size());
        for (size_t k = 0; k < edges.size(); ++k) edges[k] = itr->second[k];
        if (edges.size() < 3) continue;

        osg::Vec3Array* va2 = new osg::Vec3Array;
#if 0
        osg::DrawElementsUInt* de2 = new osg::DrawElementsUInt(GL_LINES);
#else
        osg::DrawElementsUInt* de2 = new osg::DrawElementsUInt(GL_TRIANGLES);
#endif
        osgVerse::PointList3D points; osgVerse::PointList2D points2D;
        osgVerse::EdgeList edgeIndices = osgVerse::GeometryAlgorithm::project(
            edges, -planes[itr->first].getNormal(), points, points2D);
        va2->assign(points.begin(), points.end());

#if 0
        for (size_t i = 0; i < edgeIndices.size(); ++i)
        {
            de2->push_back(edgeIndices[i].first);
            de2->push_back(edgeIndices[i].second);
        }
#else
        std::vector<size_t> indices =
            osgVerse::GeometryAlgorithm::delaunayTriangulation(points2D, edgeIndices);
        de2->assign(indices.begin(), indices.end());
#endif

        osg::Geometry* geom2 = osgVerse::createGeometry(
            va2, NULL, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), de2);
        root->addDrawable(geom2);
    }

    osg::Geometry* geom = osgVerse::createGeometry(va, NULL, NULL, de);
    root->addDrawable(geom); //osgDB::writeNodeFile(*root, "test.osg");

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
