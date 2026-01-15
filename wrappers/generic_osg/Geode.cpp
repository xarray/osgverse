#include <GenericReserializer.h>
using namespace osgVerse;

static bool readDrawables(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID drawable = ud.readObjectFromStream(is, "osg::Drawable");
        if (drawable.valid()) ud.add("addDrawable", drawable);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( Geode,
                         new osg::Geode,
                         osg::Geode,
                         "osg::Object osg::Node osg::Geode" )
{
    ADD_USER_SERIALIZER( Drawables );  // _drawables
}
