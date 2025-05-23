#include <GenericReserializer.h>
using namespace osgVerse;

REGISTER_OBJECT_WRAPPER( AnimationPathCallback,
                         new osg::AnimationPathCallback,
                         osg::AnimationPathCallback,
                         "osg::Object osg::NodeCallback osg::AnimationPathCallback" )
{
    ADD_OBJECT_SERIALIZER( AnimationPath, osg::AnimationPath, NULL );  // _animationPath
    ADD_VEC3D_SERIALIZER( PivotPoint, osg::Vec3d() );  // _pivotPoint
    ADD_BOOL_SERIALIZER( UseInverseMatrix, false );  // _useInverseMatrix
    ADD_DOUBLE_SERIALIZER( TimeOffset, 0.0 );  // _timeOffset
    ADD_DOUBLE_SERIALIZER( TimeMultiplier, 1.0 );  // _timeMultiplier
    ADD_BOOL_SERIALIZER( Pause, false );  // _pause
}
