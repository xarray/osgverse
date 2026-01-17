#include <GenericReserializer.h>
using namespace osgVerse;

#define WRAPVALUEOBJECT( TYPE, INHERITANCE_STRING, SERIALIZER_TYPE, DEFAULT) \
    namespace Wrap##TYPE \
    { \
        REGISTER_OBJECT_WRAPPER( TYPE, \
                                new osg::TYPE, \
                                osg::TYPE, \
                                INHERITANCE_STRING ) \
        { \
            SERIALIZER_TYPE( Value, DEFAULT ); \
        } \
    } \


WRAPVALUEOBJECT( BoolValueObject, "osg::Object osg::BoolValueObject", ADD_BOOL_SERIALIZER, 0)
WRAPVALUEOBJECT( CharValueObject, "osg::Object osg::CharValueObject", ADD_CHAR_SERIALIZER, 0)
WRAPVALUEOBJECT( UCharValueObject, "osg::Object osg::UCharValueObject", ADD_UCHAR_SERIALIZER, 0u)
WRAPVALUEOBJECT( ShortValueObject, "osg::Object osg::ShortValueObject", ADD_SHORT_SERIALIZER, 0)
WRAPVALUEOBJECT( UShortValueObject, "osg::Object osg::UShortValueObject", ADD_USHORT_SERIALIZER, 0u)
WRAPVALUEOBJECT( IntValueObject, "osg::Object osg::IntValueObject", ADD_INT_SERIALIZER, 0)
WRAPVALUEOBJECT( UIntValueObject, "osg::Object osg::UIntValueObject", ADD_UINT_SERIALIZER, 0u)
WRAPVALUEOBJECT( FloatValueObject, "osg::Object osg::FloatValueObject", ADD_FLOAT_SERIALIZER, 0.0f)
WRAPVALUEOBJECT( DoubleValueObject, "osg::Object osg::DoubleValueObject", ADD_DOUBLE_SERIALIZER, 0.0)

WRAPVALUEOBJECT( StringValueObject, "osg::Object osg::StringValueObject", ADD_STRING_SERIALIZER, std::string())

WRAPVALUEOBJECT( Vec2fValueObject, "osg::Object osg::Vec2fValueObject", ADD_VEC2F_SERIALIZER, osg::Vec2f())
WRAPVALUEOBJECT( Vec3fValueObject, "osg::Object osg::Vec3fValueObject", ADD_VEC3F_SERIALIZER, osg::Vec3f())
WRAPVALUEOBJECT( Vec4fValueObject, "osg::Object osg::Vec4fValueObject", ADD_VEC4F_SERIALIZER, osg::Vec4f())

WRAPVALUEOBJECT( Vec2dValueObject, "osg::Object osg::Vec2dValueObject", ADD_VEC2D_SERIALIZER, osg::Vec2d())
WRAPVALUEOBJECT( Vec3dValueObject, "osg::Object osg::Vec3dValueObject", ADD_VEC3D_SERIALIZER, osg::Vec3d())
WRAPVALUEOBJECT( Vec4dValueObject, "osg::Object osg::Vec4dValueObject", ADD_VEC4D_SERIALIZER, osg::Vec4d())

WRAPVALUEOBJECT( PlaneValueObject, "osg::Object osg::PlaneValueObject", ADD_PLANE_SERIALIZER, osg::Plane())
WRAPVALUEOBJECT( QuatValueObject, "osg::Object osg::QuatValueObject", ADD_QUAT_SERIALIZER, osg::Quat())

WRAPVALUEOBJECT( MatrixfValueObject, "osg::Object osg::MatrixfValueObject", ADD_MATRIXF_SERIALIZER, osg::Matrixf())
WRAPVALUEOBJECT( MatrixdValueObject, "osg::Object osg::MatrixdValueObject", ADD_MATRIXD_SERIALIZER, osg::Matrixd())
