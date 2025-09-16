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

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class PickHandler : public osgGA::GUIEventHandler
{
public:
    PickHandler(osgVerse::SymbolManager* sym) : _manager(sym) {}

    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::MOVE)
        {
            osg::Vec2d proj(ea.getXnormalized(), ea.getYnormalized());
            std::vector<osgVerse::Symbol*> selected = _manager->querySymbols(proj, 0.08);
            if (!_lastSelectedSymbols.empty())
            {
                for (size_t i = 0; i < _lastSelectedSymbols.size(); ++i)
                {
                    _lastSelectedSymbols[i]->color = osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
                    _lastSelectedSymbols[i]->scale = 0.15f;
                }
            }

            if (!selected.empty())
            {
                for (size_t i = 0; i < selected.size(); ++i)
                {
                    selected[i]->color = osg::Vec4(1.0f, 0.5f, 0.5f, 1.0f);
                    selected[i]->scale = 0.25f;
                }
            }
            _lastSelectedSymbols.swap(selected);
        }
        return false;
    }

protected:
    std::vector<osgVerse::Symbol*> _lastSelectedSymbols;
    osg::observer_ptr<osgVerse::SymbolManager> _manager;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Group> root = new osg::Group;

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setUpViewOnSingleScreen(0);

    osg::ref_ptr<osg::Image> iconAtlas = osgDB::readImageFile(BASE_DIR + "/textures/poi_icons.png");
    osg::ref_ptr<osg::Image> textBgAtlas = osgDB::readImageFile(BASE_DIR + "/textures/poi_textbg.png");
    if (arguments.read("--test-drawer"))
    {
        osgVerse::DrawerStyleData imgStyle(iconAtlas.get(), osgVerse::DrawerStyleData::PAD);
        std::vector<osgVerse::Drawer2D*> drawerList;
        for (size_t i = 0; i < 10; ++i)
        {
            osg::ref_ptr<osgVerse::Drawer2D> drawer = new osgVerse::Drawer2D;
            drawer->allocateImage(1024, 1024, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            drawer->loadFont("default", MISC_DIR + "LXGWFasmartGothic.otf");
            drawer->setPixelBufferObject(new osg::PixelBufferObject(drawer.get()));

            osg::Geode* geode = osg::createGeodeForImage(drawer.get());
            drawerList.push_back(drawer.get());

            osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
            mt->setMatrix(osg::Matrix::translate((float)(i % 5) * 4.0f, 0.0f, (float)(i / 5) * 2.0f));
            mt->addChild(geode); root->addChild(mt.get());
        }

        bool drawerInThreads = !arguments.read("--no-threads");
        while (!viewer.done())
        {
            double total = 0.0f;
            for (size_t i = 0; i < drawerList.size(); ++i)
            {
                osgVerse::Drawer2D* drawer = drawerList[i];
                osg::Timer_t t0 = osg::Timer::instance()->tick();
                if (drawerInThreads)
                {
                    size_t id = i, frames = viewer.getFrameStamp()->getFrameNumber();
                    drawer->startInThread([imgStyle, id, frames](osgVerse::Drawer2D* drawer)
                        {
                            drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                            drawer->drawRectangle(osg::Vec4(40, 40, 560, 400), 0.0f, 0.0f, imgStyle);

                            std::string text = "Hello " + std::to_string(id)
                                             + ": " + std::to_string(frames);
                            osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, 40.0f);
                            drawer->drawRectangle(bbox, 1.0f, 1.0f,
                                osgVerse::DrawerStyleData(osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f), true));
                            drawer->drawUtf8Text(osg::Vec2(bbox[0], bbox[1] + bbox[3]), 40.0f, text);
                        }, false);
                }
                else
                {
                    drawer->start(false);
                    drawer->fillBackground(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
                    drawer->drawRectangle(osg::Vec4(40, 40, 560, 400), 0.0f, 0.0f, imgStyle);

                    std::string text = "Hello " + std::to_string(i)
                                     + ": " + std::to_string(viewer.getFrameStamp()->getFrameNumber());
                    osg::Vec4 bbox = drawer->getUtf8TextBoundingBox(text, 40.0f);
                    drawer->drawRectangle(bbox, 1.0f, 1.0f,
                        osgVerse::DrawerStyleData(osg::Vec4(0.8f, 0.8f, 0.0f, 1.0f), true));
                    drawer->drawUtf8Text(osg::Vec2(bbox[0], bbox[1] + bbox[3]), 40.0f, text);
                }
                drawer->finish();

                osg::Timer_t t1 = osg::Timer::instance()->tick();
                total += osg::Timer::instance()->delta_m(t0, t1);
            }

            printf("DRAW TIME: %lg\n", total);
            viewer.frame();
        }
    }
    else if (arguments.read("--test-massive"))
    {
        osg::ref_ptr<osgVerse::SymbolManager> symManager = new osgVerse::SymbolManager;
        symManager->setShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "poi_symbols.vert.glsl"),
                               osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "poi_symbols.frag.glsl"));
        symManager->setFontFileName(MISC_DIR + "/LXGWFasmartGothic.otf");
        symManager->setIconAtlasImage(iconAtlas.get());
        symManager->setLodDistance(osgVerse::SymbolManager::LOD0, 10000.0);
        symManager->setLodDistance(osgVerse::SymbolManager::LOD1, 0.0);
        symManager->setLodDistance(osgVerse::SymbolManager::LOD2, 0.0);

        for (int y = 0; y < 100; ++y)
            for (int x = 0; x < 100; ++x)
            {
                osgVerse::Symbol* s = new osgVerse::Symbol;
                s->position = osg::Vec3d(x - 25, y - 25, y - 5);
                s->rotateAngle = 0.0f; s->scale = 0.15f;
                s->tiling = osg::Vec3((x % 5) / 8.0f, (y % 8) / 8.0f, 1.0f / 8.0f);
                s->color = osg::Vec4(1.0f, 1.0f, 1.0f, 0.9f);
                s->name = u8"ID_" + std::to_string(x + y * 100);
                symManager->updateSymbol(s);
            }

        osg::ref_ptr<osg::Group> symbols = new osg::Group;
        symbols->addUpdateCallback(symManager.get());
        symManager->setMainCamera(viewer.getCamera());

        root->addChild(symbols.get());
        root->addChild(osgDB::readNodeFile("axes.osgt"));
        viewer.addEventHandler(new PickHandler(symManager.get()));
        viewer.run();
    }
    else
    {

        osg::ref_ptr<osgVerse::SymbolManager> symManager = new osgVerse::SymbolManager;
        symManager->setShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR + "poi_symbols.vert.glsl"),
                               osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR + "poi_symbols.frag.glsl"));
        symManager->setFontFileName(MISC_DIR + "/LXGWFasmartGothic.otf");
        symManager->setIconAtlasImage(iconAtlas.get());
        symManager->setTextBackgroundAtlasImage(textBgAtlas.get());
        symManager->setMidDistanceTextOffset(osg::Vec3(2.0f, 0.0f, -0.001f));
        symManager->setLodDistance(osgVerse::SymbolManager::LOD0, 100.0);
        symManager->setLodDistance(osgVerse::SymbolManager::LOD1, 20.0);
        symManager->setLodDistance(osgVerse::SymbolManager::LOD2, 1.0);

        osg::Vec3Array* va = new osg::Vec3Array;
        for (int y = 0; y < 10; ++y)
            for (int x = 0; x < 10; ++x)
            {
                osgVerse::Symbol* s = new osgVerse::Symbol;
                s->position = osg::Vec3d(x - 5, y - 5, y - 5);
                s->rotateAngle = 0.0f;// osg::PI * rand() / (float)RAND_MAX;
                s->tiling = osg::Vec3((x % 5) / 8.0f, (y % 8) / 8.0f, 1.0f / 8.0f);
                s->tiling2 = osg::Vec3((x % 1) / 8.0f, (y % 8) / 8.0f, 1.0f / 8.0f);
                s->color = osg::Vec4(1.0f, 1.0f, 1.0f, 0.4f);
                s->name = u8"ID_" + std::to_string(x + y * 100);
                symManager->updateSymbol(s);

                va->push_back(s->position);
                va->push_back(s->position);
            }

        osg::ref_ptr<osg::Group> symbols = new osg::Group;
        symbols->addUpdateCallback(symManager.get());

        osg::ref_ptr<osg::Geode> symbolLines = new osg::Geode;
        {
            osg::Geometry* geom = new osg::Geometry;
            geom->setUseDisplayList(false);
            geom->setUseVertexBufferObjects(true);
            geom->setVertexArray(va);
            geom->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, va->size()));
            geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

            osg::Vec4Array* ca = new osg::Vec4Array;
            ca->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
            geom->setColorArray(ca); geom->setColorBinding(osg::Geometry::BIND_OVERALL);
            symbolLines->addDrawable(geom);
        }

        symManager->setMainCamera(viewer.getCamera());
        root->addChild(symbols.get());
        root->addChild(symbolLines.get());
        root->addChild(osgDB::readNodeFile("axes.osgt"));
        viewer.addEventHandler(new PickHandler(symManager.get()));

        while (!viewer.done())
        {
            const std::map<int, osg::ref_ptr<osgVerse::Symbol>>& symbols = symManager->getSymols();
            osg::Matrix MVP = viewer.getCamera()->getViewMatrix() * viewer.getCamera()->getProjectionMatrix();
            osg::Matrix invMVP = osg::Matrix::inverse(MVP); size_t count = 0;

            for (std::map<int, osg::ref_ptr<osgVerse::Symbol>>::const_iterator itr = symbols.begin();
                itr != symbols.end(); ++itr, count += 2)
            {
                const osgVerse::Symbol* symbol = itr->second.get();
                if (symbol->state != osgVerse::Symbol::Hidden)
                {
                    (*va)[count + 0] = symbol->getCorner2D(symManager.get(), 0) * invMVP;
                    (*va)[count + 1] = symbol->getCorner2D(symManager.get(), 1) * invMVP;
                }
                else
                {
                    (*va)[count + 0] = symbol->position;
                    (*va)[count + 1] = symbol->position;
                }
            }
            va->dirty(); symbolLines->getDrawable(0)->dirtyBound();
            viewer.frame();
        }
    }
    return 0;
}
