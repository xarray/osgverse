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

#include <marl/defer.h>
#include <marl/event.h>
#include <marl/scheduler.h>
#include <marl/waitgroup.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    marl::Scheduler scheduler(marl::Scheduler::Config::allCores());
    scheduler.bind(); defer(scheduler.unbind());

    marl::Event blockEvent(marl::Event::Mode::Manual);
    marl::schedule([=] {
        int lastFrameNumber = -1;
        while (!viewer->done())
        {
            osg::FrameStamp* fs = viewer->getFrameStamp();
            if (fs && (fs->getFrameNumber() % 300) == 0)
            {
                if (fs->getFrameNumber() == 0) continue;
                else if (lastFrameNumber == fs->getFrameNumber()) continue;
                else lastFrameNumber = fs->getFrameNumber();

                blockEvent.clear();  // Block main renderer
                std::cout << "Blocking main renderer at: "
                          << lastFrameNumber << ". Press any key...\n";
                int ch = getchar();
                blockEvent.signal();  // Recover main renderer
            }
        }
    });

    marl::WaitGroup mainRenderer(1);
    marl::schedule([=] {
        osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
        root->addChild(osgDB::readNodeFile("cessna.osg"));
        viewer->setSceneData(root.get());

        viewer->addEventHandler(new osgViewer::StatsHandler);
        viewer->addEventHandler(new osgViewer::WindowSizeHandler);
        viewer->setCameraManipulator(new osgGA::TrackballManipulator);
        viewer->setUpViewInWindow(50, 50, 800, 600);

        defer(mainRenderer.done());
        while (!viewer->done())
        {
            blockEvent.wait();
            viewer->frame();
        }
    });

    std::cout << "Starting main renderer...\n";
    blockEvent.signal();  // make the event unblocked
    mainRenderer.wait();
    return 0;
}
