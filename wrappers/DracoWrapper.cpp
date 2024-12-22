#include <osg/Version>
#include <osg/TriangleIndexFunctor>
#include <osgDB/ObjectWrapper>
#include <osgDB/InputStream>
#include <osgDB/OutputStream>
#include <readerwriter/DracoProcessor.h>
#include "Export.h"
#include <mutex>
using namespace osgVerse;

struct GeometryFinishedObjectReadCallback : public osgDB::FinishedObjectReadCallback
{
    virtual void objectRead(osgDB::InputStream&, osg::Object& obj)
    {
        osg::Geometry& geometry = static_cast<osg::Geometry&>(obj);
        if (!geometry.getUseVertexBufferObjects())
        {
            geometry.setUseDisplayList(false);
            geometry.setUseVertexBufferObjects(true);
        }
    }
};

static std::map<int, int> g_dracoParameters;
static std::mutex g_dracoMutex;

namespace osgVerse
{
    OSGVERSE_WRAPPERS_EXPORT void setEncodingDracoFlag(EncodingDracoFlag flag, int value)
    { g_dracoMutex.lock(); g_dracoParameters[(int)flag] = value; g_dracoMutex.unlock(); }
}

static bool checkCompressedData(const osgVerse::DracoGeometry& geom)
{ return true; }

static bool readCompressedData(osgDB::InputStream& is, osgVerse::DracoGeometry& geom)
{
    unsigned int dataSize = 0; is >> dataSize;
    if (dataSize == 0) return true;

    std::vector<char> data(dataSize);
    is.readCharArray(&data[0], dataSize);

    DracoProcessor dp;
    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    ss.write(&data[0], dataSize);
    return dp.decodeDracoData(ss, &geom);
}

static bool writeCompressedData(osgDB::OutputStream& os, const osgVerse::DracoGeometry& geom)
{
    DracoProcessor dp; g_dracoMutex.lock();
    for (std::map<int, int>::iterator itr = g_dracoParameters.begin();
         itr != g_dracoParameters.end(); ++itr)
    {
        if (itr->first == 0) dp.setCompressionLevel(itr->second);
        else if (itr->first == 1) dp.setPosQuantizationBits(itr->second);
        else if (itr->first == 2) dp.setNormalQuantizationBits(itr->second);
        else if (itr->first == 3) dp.setUvQuantizationBits(itr->second);
    }
    g_dracoMutex.unlock();

    std::stringstream ss(std::ios::in | std::ios::out | std::ios::binary);
    if (dp.encodeDracoData(ss, (osg::Geometry*)&geom))
    {
        std::string data = ss.str();
        unsigned int dataSize = data.size();
        os << dataSize; if (dataSize == 0) return true;
        os.writeCharArray(&data[0], dataSize); return true;
    }
    return false;
}

#ifdef VERSE_USE_DRACO

#   if OSG_VERSION_GREATER_THAN(3, 4, 1)
REGISTER_OBJECT_WRAPPER(DracoGeometry,
                        new osgVerse::DracoGeometry,
                        osgVerse::DracoGeometry,  // ignore osg::Geometry to NOT serialize vertices and primitives
                        "osg::Object osg::Node osg::Drawable osgVerse::DracoGeometry")
{
    {
        UPDATE_TO_VERSION_SCOPED(154)
            ADDED_ASSOCIATE("osg::Node")
    }
    ADD_USER_SERIALIZER(CompressedData);
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#   else
REGISTER_OBJECT_WRAPPER(DracoGeometry,
                        new osgVerse::DracoGeometry,
                        osgVerse::DracoGeometry,  // ignore osg::Geometry to NOT serialize vertices and primitives
                        "osg::Object osg::Drawable osgVerse::DracoGeometry")
{
    ADD_USER_SERIALIZER(CompressedData);
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#   endif

#else  // VERSE_USE_DRACO

#   if OSG_VERSION_GREATER_THAN(3, 4, 1)
REGISTER_OBJECT_WRAPPER(DracoGeometry,
                        new osgVerse::DracoGeometry,
                        osgVerse::DracoGeometry,
                        "osg::Object osg::Node osg::Drawable osg::Geometry osgVerse::DracoGeometry")
{
    {
        UPDATE_TO_VERSION_SCOPED(154)
            ADDED_ASSOCIATE("osg::Node")
    }
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#   else
REGISTER_OBJECT_WRAPPER(DracoGeometry,
                        new osgVerse::DracoGeometry,
                        osgVerse::DracoGeometry,
                        "osg::Object osg::Drawable osg::Geometry osgVerse::DracoGeometry")
{
    wrapper->addFinishedObjectReadCallback(new GeometryFinishedObjectReadCallback());
}
#   endif

#endif  // VERSE_USE_DRACO
