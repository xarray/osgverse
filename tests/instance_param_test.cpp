#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/VertexAttribDivisor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

osg::Geometry* createInstancedGeometry(bool useVertexAttribDivisor)
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
    va->push_back(osg::Vec3(-1.0f, 0.0f, -1.0f)); ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    va->push_back(osg::Vec3(1.0f, 0.0f, -1.0f)); ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    va->push_back(osg::Vec3(1.0f, 1.0f, 1.0f)); ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
    va->push_back(osg::Vec3(-1.0f, 1.0f, 1.0f)); ca->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));

    osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
    de->push_back(0); de->push_back(1); de->push_back(2);
    de->push_back(0); de->push_back(2); de->push_back(3);
    de->setNumInstances(512 * 512);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get()); geom->setColorArray(ca.get());
    geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
    geom->addPrimitiveSet(de.get());
    return geom.release();
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addChild(createInstancedGeometry(true));

    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setUpViewInWindow(50, 50, 800, 600);
    viewer->setSceneData(root.get());
    while (!viewer->done())
    {
        viewer->frame();
    }
    return 0;
}
