#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( ImageStream,
                         new osg::ImageStream,
                         osg::ImageStream,
                         "osg::Object osg::BufferData osg::Image osg::ImageStream" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::BufferData")
    }
    BEGIN_ENUM_SERIALIZER( LoopingMode, NO_LOOPING );
        ADD_ENUM_VALUE( NO_LOOPING );
        ADD_ENUM_VALUE( LOOPING );
    END_ENUM_SERIALIZER();  // _loopingMode

    ADD_OBJECT_LIST_SERIALIZER( AudioStreams, osg::AudioStream );  // _audioStreams
}
