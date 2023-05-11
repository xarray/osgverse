#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/PrimitiveSetIndirect>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::Vec3Array* va = new osg::Vec3Array;
    osg::Vec4Array* ca = new osg::Vec4Array;
    for (size_t i = 0; i < 100; ++i)
    {
        va->push_back(osg::Vec3((float)i, 0.0f, 0.0f));
        va->push_back(osg::Vec3((float)i, 0.0f, 1.0f));
        ca->push_back(osg::Vec4((float)i * 0.01f, 0.0f, 0.0f, 1.0f));
        ca->push_back(osg::Vec4((float)i * 0.01f, 1.0f, 0.0f, 1.0f));
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(va);
    geom->setColorArray(ca);
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addDrawable(geom.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
