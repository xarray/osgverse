#include <GenericReserializer.h>
using namespace osgVerse;

static bool readMasks(InputStream& is, InputUserData& ud)
{
    if ( is.getFileVersion() > 96 )
    {
        unsigned int mask0, mask1;
        is >> mask0 >> mask1;
        ud.add("setMask", mask0, 0);
        ud.add("setMask", mask1, 1);
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( SampleMaski,
                         new osg::SampleMaski,
                         osg::SampleMaski,
                         "osg::Object osg::StateAttribute osg::SampleMaski" )
{
    ADD_USER_SERIALIZER( Masks );  //
}
