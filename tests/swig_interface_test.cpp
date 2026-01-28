#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osg/Version>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <script/Entry.h>
#include <wrappers/Export.h>
#include <iostream>
#include <sstream>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

static std::string getPropertyTypeName(const std::string& clsName, osgDB::BaseSerializer::Type type)
{
    switch (type)
    {
    case osgDB::BaseSerializer::RW_OBJECT: return "Object";
    case osgDB::BaseSerializer::RW_IMAGE: return "Image";
    case osgDB::BaseSerializer::RW_BOOL: return "bool";
    case osgDB::BaseSerializer::RW_CHAR: return "char";
    case osgDB::BaseSerializer::RW_UCHAR: return "unsigned char";
    case osgDB::BaseSerializer::RW_SHORT: return "short";
    case osgDB::BaseSerializer::RW_USHORT: return "unsigned short";
    case osgDB::BaseSerializer::RW_INT: return "int";
    case osgDB::BaseSerializer::RW_GLENUM: return "GLenum";
    case osgDB::BaseSerializer::RW_UINT: return "unsigned int";
    case osgDB::BaseSerializer::RW_FLOAT: return "float";
    case osgDB::BaseSerializer::RW_DOUBLE: return "double";
    case osgDB::BaseSerializer::RW_QUAT: return "Quat";
    case osgDB::BaseSerializer::RW_VEC2F: return "Vec2";
    case osgDB::BaseSerializer::RW_VEC3F: return "Vec3";
    case osgDB::BaseSerializer::RW_VEC4F: return "Vec4";
    case osgDB::BaseSerializer::RW_VEC2D: return "Vec2d";
    case osgDB::BaseSerializer::RW_VEC3D: return "Vec3d";
    case osgDB::BaseSerializer::RW_VEC4D: return "Vec4d";
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    case osgDB::BaseSerializer::RW_VEC2B: return "Vec2b";
    case osgDB::BaseSerializer::RW_VEC3B: return "Vec3b";
    case osgDB::BaseSerializer::RW_VEC4B: return "Vec4b";
    case osgDB::BaseSerializer::RW_VEC2UB: return "Vec2ub";
    case osgDB::BaseSerializer::RW_VEC3UB: return "Vec3ub";
    case osgDB::BaseSerializer::RW_VEC4UB: return "Vec4ub";
    case osgDB::BaseSerializer::RW_VEC2S: return "Vec2s";
    case osgDB::BaseSerializer::RW_VEC3S: return "Vec3s";
    case osgDB::BaseSerializer::RW_VEC4S: return "Vec4s";
    case osgDB::BaseSerializer::RW_VEC2US: return "Vec2us";
    case osgDB::BaseSerializer::RW_VEC3US: return "Vec3us";
    case osgDB::BaseSerializer::RW_VEC4US: return "Vec4us";
    case osgDB::BaseSerializer::RW_VEC2I: return "Vec2i";
    case osgDB::BaseSerializer::RW_VEC3I: return "Vec3i";
    case osgDB::BaseSerializer::RW_VEC4I: return "Vec4i";
    case osgDB::BaseSerializer::RW_VEC2UI: return "Vec2ui";
    case osgDB::BaseSerializer::RW_VEC3UI: return "Vec3ui";
    case osgDB::BaseSerializer::RW_VEC4UI: return "Vec4ui";
#endif
    case osgDB::BaseSerializer::RW_MATRIXF: return "Matrixf";
    case osgDB::BaseSerializer::RW_MATRIXD: return "Matrixd";
    case osgDB::BaseSerializer::RW_MATRIX: return "Matrix";
    case osgDB::BaseSerializer::RW_STRING: return "std::string";
    case osgDB::BaseSerializer::RW_ENUM: return "Enum";
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
    case osgDB::BaseSerializer::RW_VECTOR:
        if (clsName == "FloatArray") return "std::vector<float>";
        else if (clsName == "Vec2Array") return "std::vector<osg::Vec2>";
        else if (clsName == "Vec3Array") return "std::vector<osg::Vec3>";
        else if (clsName == "Vec4Array") return "std::vector<osg::Vec4>";
        else if (clsName == "DoubleArray") return "std::vector<double>";
        else if (clsName == "Vec2dArray") return "std::vector<osg::Vec2d>";
        else if (clsName == "Vec3dArray") return "std::vector<osg::Vec3d>";
        else if (clsName == "Vec4dArray") return "std::vector<osg::Vec4d>";
        else if (clsName == "DrawElementsUByte") return "std::vector<unsigned char>";
        else if (clsName == "DrawElementsUShort") return "std::vector<unsigned short>";
        else if (clsName == "DrawElementsUInt") return "std::vector<unsigned int>";
        else return "std::vector<Object>";
        break;
#endif
    default:
        //RW_PLANE, RW_BOUNDINGBOXF, RW_BOUNDINGBOXD, RW_BOUNDINGSPHEREF, RW_BOUNDINGSPHERED
        return "Unknown_" + std::to_string((int)type);
    }
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgVerse::updateOsgBinaryWrappers();
    std::string ns = "osg";

    osg::ref_ptr<osgVerse::LibraryEntry> entry = new osgVerse::LibraryEntry(ns);
    const std::set<std::string>& classes = entry->getClasses();
    for (std::set<std::string>::const_iterator it = classes.begin(); it != classes.end(); ++it)
    {
        const std::string& clsName = *it;
        std::vector<osgVerse::LibraryEntry::Property> props = entry->getPropertyNames(clsName);
        std::vector<osgVerse::LibraryEntry::Method> methods = entry->getMethodNames(clsName);

        std::stringstream ss;
        ss << "namespace " << ns << " {" << std::endl;
        ss << "  class " << clsName << " : public Object {" << std::endl << "  public:" << std::endl;
        ss << "    " << clsName << "();" << std::endl << "    ~" << clsName << "();" << std::endl;
        for (size_t i = 0; i < props.size(); ++i)
        {
            const osgVerse::LibraryEntry::Property& prop = props[i];
            if (prop.outdated || prop.ownerClass == "Object") continue;

            std::string typeName = getPropertyTypeName(clsName, prop.type);
            ss << "    void set" << prop.name << "(" << typeName << " v);" << std::endl;
            ss << "    " << typeName << " get" << prop.name << "();" << std::endl;
        }

        for (size_t i = 0; i < methods.size(); ++i)
        {
            const osgVerse::LibraryEntry::Method& m = methods[i]; if (m.outdated) continue;
            //
        }
        ss << "  }" << std::endl << "}" << std::endl;

        std::cout << ss.str();  // TODO
    }
    return 0;
}
