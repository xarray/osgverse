#include <GenericReserializer.h>
using namespace osgVerse;

#define MATERIAL_FUNC( PROP, TYPE ) \
    static bool read##PROP(InputStream& is, InputUserData& ud) { \
        bool frontAndBack; TYPE value1, value2; \
        is >> frontAndBack; \
        is >> is.PROPERTY("Front") >> value1; \
        is >> is.PROPERTY("Back") >> value2; \
        if ( frontAndBack ) \
            ud.add("set" #PROP, GL_FRONT_AND_BACK, value1); \
        else { \
            ud.add("set" #PROP, GL_FRONT, value1); \
            ud.add("set" #PROP, GL_BACK, value2); \
        } \
        return true; \
    }

MATERIAL_FUNC( Ambient, osg::Vec4f )
MATERIAL_FUNC( Diffuse, osg::Vec4f )
MATERIAL_FUNC( Specular, osg::Vec4f )
MATERIAL_FUNC( Emission, osg::Vec4f )
MATERIAL_FUNC( Shininess, float )

REGISTER_OBJECT_WRAPPER( Material,
                         new osg::Material,
                         osg::Material,
                         "osg::Object osg::StateAttribute osg::Material" )
{
    BEGIN_ENUM_SERIALIZER( ColorMode, OFF );
        ADD_ENUM_VALUE( AMBIENT );
        ADD_ENUM_VALUE( DIFFUSE );
        ADD_ENUM_VALUE( SPECULAR );
        ADD_ENUM_VALUE( EMISSION );
        ADD_ENUM_VALUE( AMBIENT_AND_DIFFUSE );
        ADD_ENUM_VALUE( OFF );
    END_ENUM_SERIALIZER();  // _colorMode

    ADD_USER_SERIALIZER( Ambient );  // _ambient
    ADD_USER_SERIALIZER( Diffuse );  // _diffuse
    ADD_USER_SERIALIZER( Specular );  // _specular
    ADD_USER_SERIALIZER( Emission );  // _emission
    ADD_USER_SERIALIZER( Shininess );  // _shininess
}
