#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <tinydir.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class RemoveDataPathVisitor : public osg::NodeVisitor
{
public:
    virtual void apply(osg::PagedLOD& node)
    {
        std::cout << "DB Path: " << node.getDatabasePath() << " => empty\n";
        node.setDatabasePath("");  traverse(node);
    }
};

int main(int argc, char** argv)
{
    osgDB::Registry::instance()->loadLibrary(
        osgDB::Registry::instance()->createLibraryNameForExtension("verse_leveldb"));
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->setName("PlodGridRoot");

    if (argc < 2)
    {
        //std::string prefix = "F:/DataSet/FactoryDemo/Data/", dbBase = "leveldb://factory.db/";
        std::string prefix = "I:/sdk/data/HongKong/", dbBase = "leveldb://hongkong.db/";
        tinydir_dir dir; tinydir_open(&dir, prefix.c_str());
        while (dir.has_next)
        {
            tinydir_file file; tinydir_readfile(&dir, &file);
            if (file.is_dir)
            {
                std::string dirName = file.name, rootTileName;
                if (dirName.find("Tile") != dirName.npos)
                    rootTileName = prefix + dirName + "/" + dirName + ".osgb";
                //else if (dirName.find("tile") != dirName.npos)
                //    rootTileName = prefix + dirName + "/Data/Model/Model_with_transform.osgb";
                if (rootTileName.empty()) { tinydir_next(&dir); continue; }

                osgDB::DirectoryContents osgbFiles = osgDB::getDirectoryContents(prefix + dirName);
                for (size_t i = 0; i < osgbFiles.size(); ++i)
                {
                    std::string tileName = prefix + dirName + "/" + osgbFiles[i];
                    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(tileName);
                    if (node.valid())
                    {
                        RemoveDataPathVisitor rdp;
                        rdp.setTraversalMode(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN);
                        node->accept(rdp);

                        std::string dbFileName = dbBase + dirName + "/" + osgbFiles[i];
                        osgDB::writeNodeFile(*node, dbFileName,
                                             new osgDB::Options("WriteImageHint=IncludeFile"));
                        std::cout << "Re-saving " << osgbFiles[i] << "\n";
                    }
                }

                osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(rootTileName);
                if (node.valid())
                {
                    osg::ref_ptr<osg::PagedLOD> plod = new osg::PagedLOD;
                    plod->setDatabasePath(dbBase);
                    plod->setCenterMode(osg::LOD::USER_DEFINED_CENTER);
                    plod->setCenter(node->getBound().center());
                    plod->setRadius(node->getBound().radius());
                    plod->addChild(new osg::Node);
                    plod->setFileName(1, dirName + "/" + dirName + ".osgb");
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
        osgDB::writeNodeFile(*root, "HongKong1.osg");
    }
    else
        root->addChild(osgDB::readNodeFile(argv[1]));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
