#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <VerseCommon.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Rasterizer.h>
#include <pipeline/Pipeline.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFile("lz.osg");
    osg::ref_ptr<osg::Node> cessna = osgDB::readNodeFile("cessna.osg");
    if (!terrain || !cessna) return 1;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    terrain->setName("TERRAIN"); root->addChild(terrain.get());
    cessna->setName("CESSNA"); root->addChild(cessna.get());
    
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    osg::ref_ptr<osgVerse::UserOccluder> occ1 = new osgVerse::UserOccluder(*terrain);
    osg::ref_ptr<osgVerse::UserOccluder> occ2 = new osgVerse::UserOccluder(*cessna);

    osg::ref_ptr<osgVerse::UserRasterizer> rasterizer = new osgVerse::UserRasterizer(1280, 720);
    rasterizer->addOccluder(occ1.get()); rasterizer->addOccluder(occ2.get());

    std::vector<float> depthData;
    while (!viewer.done())
    {
        osg::Matrix mvp = viewer.getCamera()->getViewMatrix()
                        * viewer.getCamera()->getProjectionMatrix();
        osg::Vec3 cameraPos = osg::Vec3() * viewer.getCamera()->getInverseViewMatrix();
        rasterizer->setModelViewProjection(mvp);
        rasterizer->render(cameraPos, depthData);

        viewer.frame();
    }

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1280, 720, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    {
        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        for (int y = 0; y < image->t(); ++y)
            for (int x = 0; x < image->s(); ++x)
            {
                unsigned char value = (unsigned char)(255.0f * depthData[y * image->s() + x]);
                *(ptr + y * image->s() + x) = osg::Vec4ub(value, value, value, 255);
            }
    }
    osgDB::writeImageFile(*image, "test_occlusion.png");
    return 0;
}
