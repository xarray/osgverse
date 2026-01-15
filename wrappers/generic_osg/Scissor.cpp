#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    int x, y, w, h;
    is >> x >> y >> w >> h;
    ud.add("setScissor", x, y, w, h);
    return true;
}

REGISTER_OBJECT_WRAPPER( Scissor,
                         new osg::Scissor,
                         osg::Scissor,
                         "osg::Object osg::StateAttribute osg::Scissor" )
{
    ADD_USER_SERIALIZER( Area );  // _x, _y, _width, _height
}
