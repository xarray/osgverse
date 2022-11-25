#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <tinydir.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName("PlodGridRoot");

    std::string prefix = "G:/HongKong/";
    tinydir_dir dir; tinydir_open(&dir, prefix.c_str());
    while (dir.has_next)
    {
        tinydir_file file; tinydir_readfile(&dir, &file);
        if (file.is_dir)
        {
            std::string dirName = file.name, fullName;
            if (dirName.find("Tile") != dirName.npos)
                ;// fullName = prefix + dirName + "/" + dirName + ".osgb";
            else if (dirName.find("tile") != dirName.npos)
                fullName = prefix + dirName + "/Data/Model/Model_with_transform.osgb";
            if (fullName.empty()) { tinydir_next(&dir); continue; }

            osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(fullName);
            if (node.valid())
            {
                osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                plod->setCenter(node->getBound().center());
                plod->setRadius(node->getBound().radius());
                plod->addChild(new osg::Node);
                plod->setFileName(1, fullName);
                plod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
                plod->setRange(0, 150000.0f, FLT_MAX);
                plod->setRange(1, 0.0f, 150000.0f);
                plod->setName(dirName);
                root->addChild(plod.get());
            }
            std::cout << dirName << " added...\n";
        }
        tinydir_next(&dir);
    }
    tinydir_close(&dir);
    osgDB::writeNodeFile(*root, "HongKong.osg");

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
