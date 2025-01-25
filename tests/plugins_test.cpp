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

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    std::string pluginExt = "verse_vdb";
    arguments.read("--ext", pluginExt);

    osgDB::ReaderWriter* rw = osgDB::Registry::instance()->getReaderWriterForExtension(pluginExt);
    if (rw != NULL) { std::cout << rw->className() << " found.\n"; return 0; }

    std::string libName = osgDB::Registry::instance()->createLibraryNameForExtension(pluginExt);
    osgDB::Registry::LoadStatus status = osgDB::Registry::instance()->loadLibrary(libName);
    if (status != osgDB::Registry::NOT_LOADED) { std::cout << libName << " loaded.\n"; return 0; }

    std::cout << "Failed to load " << pluginExt << "\n";
#if WIN32
    std::cout << "Use dumpbin /dependents " << libName << " to check dependency DLLs\n";
#endif
    return 0;
}
