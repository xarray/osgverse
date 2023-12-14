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
#include <modeling/CsgBoolean.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> nodeA = (argc < 2)
                                  ? osgDB::readNodeFile("cow.osg") : osgDB::readNodeFile(argv[1]);
#if false
    //osg::ref_ptr<osg::Node> nodeB = osgDB::readNodeFile("cow.osg.(-1,-0.5,0).trans");
    osg::ref_ptr<osg::Geode> nodeB = new osg::Geode;
    //nodeB->addDrawable(osgVerse::createEllipsoid(osg::Vec3(0.0f, -0.5f, 0.0f), 1.0f, 1.0f, 3.0f));
    nodeB->addDrawable(osgVerse::createPrism(osg::Vec3(0.0f, -8.0f, -10.0f), 10.0f, 10.0f, 20.0f));

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(osgVerse::CsgBoolean::process(
        osgVerse::CsgBoolean::A_NOT_B, nodeA.get(), nodeB.get()));
    //root->addChild(nodeA.get()); root->addChild(nodeB.get());
#else
    osg::Polytope polytope;
    {
        osg::ComputeBoundsVisitor cbv;
        nodeA->accept(cbv);

        osg::BoundingBox bb = cbv.getBoundingBox();
        osg::Vec3 maxPt((bb._max.x() + bb._min.x()) * 0.5f, bb._max.y(), bb._max.z());
        polytope.setToBoundingBox(osg::BoundingBox(bb._min, maxPt));
    }

    std::vector<osgVerse::IntersectionResult> results =
        osgVerse::findAllIntersections(nodeA.get(), polytope);
    osg::Vec3Array *va = new osg::Vec3Array, *va2 = new osg::Vec3Array;
    osg::DrawElementsUInt* de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (size_t k = 0; k < results.size(); ++k)
    {
        osgVerse::IntersectionResult& ir = results[k];
        std::vector<osg::Vec3d> origPt = ir.intersectPoints;

        std::vector<osg::Vec3> ptIn, ptOut, boundary;
        for (size_t i = 0; i < origPt.size(); ++i)
        {
            ptIn.push_back(origPt[i]);
            if (ir.ratioList[i] == 0.0) boundary.push_back(ptIn.back());
        };
        osgVerse::GeometryAlgorithm::reorderPointsInPlane(ptIn, ptOut);
        if (boundary.size() == 2) { va2->push_back(boundary[0]); va2->push_back(boundary[1]); }

        size_t numPt = ptOut.size(), i0 = va->size();
        for (size_t p = 0; p < numPt; ++p) va->push_back(ptOut[p] * ir.matrix);
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

    osg::Geometry* geom = osgVerse::createGeometry(va, NULL, NULL, de);
    osg::Geometry* geom2 = osgVerse::createGeometry(va2, NULL, osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f),
                                                    new osg::DrawArrays(GL_LINES, 0, va2->size()));


    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable(geom); root->addDrawable(geom2);
    //osgDB::writeNodeFile(*root, "test.osg");
#endif

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
