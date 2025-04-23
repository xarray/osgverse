#include <GenericReserializer.h>
using namespace osgVerse;

static bool readTimeControlPointMap( InputStream& is, InputUserData& ud )
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            double time = 0.0;
            osg::Vec3d pos, scale;
            osg::Quat rot;
            is >> is.PROPERTY("Time") >> time >> is.BEGIN_BRACKET;
            is >> is.PROPERTY("Position") >> pos;
            is >> is.PROPERTY("Rotation") >> rot;
            is >> is.PROPERTY("Scale") >> scale;
            is >> is.END_BRACKET;
            ud.add("readTimeControlPointMap", time, pos, rot, scale);
        }
        is >> is.END_BRACKET;
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( AnimationPath,
                         new osg::AnimationPath,
                         osg::AnimationPath,
                         "osg::Object osg::AnimationPath" )
{
    ADD_USER_SERIALIZER( TimeControlPointMap );  // _timeControlPointMap

    BEGIN_ENUM_SERIALIZER( LoopMode, LOOP );
        ADD_ENUM_VALUE( SWING );
        ADD_ENUM_VALUE( LOOP );
        ADD_ENUM_VALUE( NO_LOOPING );
    END_ENUM_SERIALIZER();  //_loopMode
}
