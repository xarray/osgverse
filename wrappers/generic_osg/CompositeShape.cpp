#include <GenericReserializer.h>
using namespace osgVerse;

static bool readChildren(InputStream& is, InputUserData& ud)
{
    unsigned int size = 0; is >> size >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID child = ud.readObjectFromStream(is, "osg::Shape");
        if (child.valid()) ud.add("addChild", child);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( CompositeShape,
                         new osg::CompositeShape,
                         osg::CompositeShape,
                         "osg::Object osg::Shape osg::CompositeShape" )
{
    ADD_OBJECT_SERIALIZER( Shape, osg::Shape, NULL );  // _shape
    ADD_USER_SERIALIZER( Children );  //_children
}
