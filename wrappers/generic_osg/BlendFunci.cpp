#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( BlendFunci,
                         new osg::BlendFunci,
                         osg::BlendFunci,
                         "osg::Object osg::StateAttribute osg::BlendFunc osg::BlendFunci" )
{
    ADD_UINT_SERIALIZER( Index, 0 );
}
