#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ColorMaski,
                         new osg::ColorMaski,
                         osg::ColorMaski,
                         "osg::Object osg::StateAttribute osg::ColorMask osg::ColorMaski" )
{
    ADD_UINT_SERIALIZER( Index, 0 );
}
