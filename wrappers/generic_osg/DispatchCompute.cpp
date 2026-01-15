#include <GenericReserializer.h>
using namespace osgVerse;

// _numGroupsX/Y/Z
static bool readComputeGroups(InputStream& is, InputUserData& ud)
{
    GLint numX = 0, numY = 0, numZ = 0;
    is >> numX >> numY >> numZ;
    ud.add("setComputeGroups", numX, numY, numZ);
    return true;
}

REGISTER_OBJECT_WRAPPER( DispatchCompute,
                         new osg::DispatchCompute,
                         osg::DispatchCompute,
                         "osg::Object osg::Node osg::Drawable osg::DispatchCompute" )
{
        ADD_USER_SERIALIZER( ComputeGroups );  // _numGroupsX/Y/Z
}
