#include <osg/io_utils>
#include <osg/Texture2D>
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
#include <pipeline/SymbolManager.h>
#include <pipeline/Drawer2D.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    //osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Images/osg256.png");
    osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;

    drawer->start(true);
    drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.5f));
    drawer->loadFont("default", MISC_DIR "/SourceHanSansHWSC-Regular.otf");
    //drawer->drawText(osg::Vec2(100, 100), 40.0f, L"Hello World");
    drawer->drawUtf8Text(osg::Vec2(100, 100), 40.0f, "Hello World");
    drawer->finish();
    osgDB::writeImageFile(*drawer, "drawer.png");

    const osg::Vec3 colors[12] = {
        osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(1.0f, 1.0f, 0.0f), osg::Vec3(1.0f, 0.0f, 1.0f),
        osg::Vec3(0.0f, 1.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 1.0f), osg::Vec3(0.0f, 0.0f, 1.0f),
        osg::Vec3(1.0f, 0.0f, 0.5f), osg::Vec3(1.0f, 0.5f, 0.0f), osg::Vec3(0.5f, 0.0f, 1.0f),
        osg::Vec3(0.0f, 0.5f, 1.0f), osg::Vec3(0.0f, 1.0f, 0.5f), osg::Vec3(1.0f, 0.0f, 0.5f)
    };

    osg::ref_ptr<osgVerse::SymbolManager> symManager = new osgVerse::SymbolManager;
    for (int y = 0; y < 100; ++y)
        for (int x = 0; x < 100; ++x)
        {
            osgVerse::Symbol* s = new osgVerse::Symbol;
            s->position = osg::Vec3d(x - 50, y - 50, y - 50); s->scale = 0.01f;
            s->rotateAngle = osg::PI * rand() / (float)RAND_MAX;
            s->color = colors[(x * 100 + y) % 12];
            symManager->updateSymbol(s);
        }

    osg::ref_ptr<osg::Group> symbols = new osg::Group;
    symbols->addUpdateCallback(symManager.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(symbols.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    symManager->setMainCamera(viewer.getCamera());
    return viewer.run();
}
