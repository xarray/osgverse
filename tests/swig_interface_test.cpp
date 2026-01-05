#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ClassInterface>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Utilities.h>
#include <script/Entry.h>
#include <wrappers/Export.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osgVerse::LibraryEntry> entry = new osgVerse::LibraryEntry("osg");
    const std::set<std::string>& classes = entry->getClasses();
    for (std::set<std::string>::const_iterator it = classes.begin(); it != classes.end(); ++it)
    {
        const std::string& clsName = *it;
        std::vector<osgVerse::LibraryEntry::Property> props = entry->getPropertyNames(clsName);
        std::vector<osgVerse::LibraryEntry::Method> methods = entry->getMethodNames(clsName);

        std::cout << clsName << ": " << props.size() << "\n";  // TBD...
    }
    return 0;
}
