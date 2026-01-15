#include <GenericReserializer.h>
using namespace osgVerse;

static bool readRestartIndex(InputStream& is, InputUserData& ud)
{
    if ( is.getFileVersion() > 97 )
    {
        unsigned int restartIndex;
        is >> restartIndex;
        ud.add("setRestartIndex", restartIndex);
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( PrimitiveRestartIndex,
                         new osg::PrimitiveRestartIndex,
                         osg::PrimitiveRestartIndex,
                         "osg::Object osg::StateAttribute osg::PrimitiveRestartIndex" )
{
    ADD_USER_SERIALIZER( RestartIndex );
}
