#include <GenericReserializer.h>
using namespace osgVerse;

#if 1

static bool readColorMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        float key = 0.0f;
        osg::Vec4d value;
        is >> key >> value;
        ud.add("setColor", key, value);
    }
    is >> is.END_BRACKET;
    return true;
}

#endif


REGISTER_OBJECT_WRAPPER( TransferFunction1D,
                         new osg::TransferFunction1D,
                         osg::TransferFunction1D,
                         "osg::Object osg::TransferFunction osg::TransferFunction1D" )
{
#if 1
    ADD_USER_SERIALIZER( ColorMap );  // _colorMap
#else
    ADD_MAP_SERIALIZER(ColorMap, osg::TransferFunction1D::ColorMap, osgDB::BaseSerializer::RW_FLOAT, osgDB::BaseSerializer::RW_VEC4F);
#endif
}
