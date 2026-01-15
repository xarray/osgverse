#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMask(InputStream& is, InputUserData& ud)
{
    char mask[128] = {0};
    if ( is.isBinary() )
    {
        unsigned int size; is >> size;
        is.readCharArray( mask, size );
    }
    else
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<128; ++i )
        {
            is >> std::hex >> mask[i] >> std::dec;
        }
        is >> is.END_BRACKET;
    }
    ud.add("setMask", (GLubyte*)mask);
    return true;
}

REGISTER_OBJECT_WRAPPER( PolygonStipple,
                         new osg::PolygonStipple,
                         osg::PolygonStipple,
                         "osg::Object osg::StateAttribute osg::PolygonStipple" )
{
    ADD_USER_SERIALIZER( Mask );  // _mask
}
