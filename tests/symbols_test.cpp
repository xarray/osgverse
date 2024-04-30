#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/MatrixTransform>
#include <osgDB/ConvertUTF>
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
    /*osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;

    drawer->start(true);
    drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.5f));
    drawer->loadFont("default", MISC_DIR "/SourceHanSansHWSC-Regular.otf");
    //drawer->drawText(osg::Vec2(100, 100), 40.0f, L"Hello World");
    drawer->drawUtf8Text(osg::Vec2(100, 100), 40.0f, "Hello World");
    drawer->finish();
    osgDB::writeImageFile(*drawer, "drawer.png");*/

    osg::ref_ptr<osg::Image> iconAtlas = osgDB::readImageFile(MISC_DIR "poi_icons.png");
    osg::ref_ptr<osg::Image> textBgAtlas = osgDB::readImageFile(MISC_DIR "poi_textbg.png");

    osg::ref_ptr<osgVerse::SymbolManager> symManager = new osgVerse::SymbolManager;
    symManager->setFontFileName(MISC_DIR "/SourceHanSansHWSC-Regular.otf");
    symManager->setIconAtlasImage(iconAtlas.get());
    symManager->setTextBackgroundAtlasImage(textBgAtlas.get());
    symManager->setMidDistanceTextOffset(osg::Vec3(2.0f, 0.0f, -0.001f));
    symManager->setLodDistance(osgVerse::SymbolManager::LOD0, 100.0);
    symManager->setLodDistance(osgVerse::SymbolManager::LOD1, 20.0);
    symManager->setLodDistance(osgVerse::SymbolManager::LOD2, 1.0);

    for (int y = 0; y < 10; ++y)
        for (int x = 0; x < 10; ++x)
        {
            osgVerse::Symbol* s = new osgVerse::Symbol;
            s->position = osg::Vec3d(x - 5, y - 5, y - 5);
            s->rotateAngle = 0.0f;// osg::PI * rand() / (float)RAND_MAX;
            s->texTiling = osg::Vec3((x % 5) / 8.0f, (y % 8) / 8.0f, 1.0f / 8.0f);
            s->texTiling2 = osg::Vec3((x % 1) / 8.0f, (y % 8) / 8.0f, 1.0f / 8.0f);
            s->name = u8"ID_" + std::to_string(x + y * 100);
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
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);

    symManager->setMainCamera(viewer.getCamera());
    return viewer.run();
}
