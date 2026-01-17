#include <GenericReserializer.h>
using namespace osgVerse;

static bool readArea(InputStream& is, InputUserData& ud)
{
    double x, y, w, h;
    is >> x >> y >> w >> h;
    ud.add("setViewport", x, y, w, h);
    return true;
}

REGISTER_OBJECT_WRAPPER( Viewport,
                         new osg::Viewport,
                         osg::Viewport,
                         "osg::Object osg::StateAttribute osg::Viewport" )
{
    ADD_USER_SERIALIZER( Area );  // _x, _y, _width, _height
}
