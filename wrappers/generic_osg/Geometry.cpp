#include <GenericReserializer.h>
using namespace osgVerse;

static void add_user_value_func_AttributeBinding(IntLookup* lookup)
{
    lookup->add("BIND_OFF", 0);                 // ADD_USER_VALUE("ADD_USER_VALUE( BIND_OFF );
    lookup->add("BIND_OVERALL", 1);             // ADD_USER_VALUE( BIND_OVERALL );
    lookup->add("BIND_PER_PRIMITIVE_SET", 2);   // ADD_USER_VALUE( BIND_PER_PRIMITIVE_SET );
    lookup->add("BIND_PER_PRIMITIVE", 3);       // ADD_USER_VALUE( BIND_PER_PRIMITIVE );
    lookup->add("BIND_PER_VERTEX", 4);          // ADD_USER_VALUE( BIND_PER_VERTEX );
 }
static UserLookupTableProxy s_user_lookup_table_AttributeBinding(&add_user_value_func_AttributeBinding);
USER_READ_FUNC( AttributeBinding, readAttributeBinding )

static ObjectTypeAndID readArray(InputStream& is, InputUserData& ud)
{
    ObjectTypeAndID array;
    bool hasArray = false;
    is >> is.PROPERTY("Array") >> hasArray;
    if ( hasArray ) array = ud.readObjectFromStream(is, "osg::Array");

    bool hasIndices = false;
    is >> is.PROPERTY("Indices") >> hasIndices;
    if ( hasIndices )
    {
        ObjectTypeAndID indices = ud.readObjectFromStream(is, "osg::IndexArray");
        if (array.valid() && indices.valid()) ud.add(array, "setUserData", &indices);
    }

    is >> is.PROPERTY("Binding");
    int binding = readAttributeBinding(is);
    if (array.valid()) ud.add(array, "setBinding", binding);

    int normalizeValue = 0;
    is >> is.PROPERTY("Normalize") >> normalizeValue;
    if (array.valid()) ud.add(array, "setNormalize", normalizeValue != 0);
    return array;
}

#define ADD_ARRAYDATA_FUNCTIONS( ORIGINAL_PROP, PROP ) \
    static bool read##ORIGINAL_PROP(InputStream& is, InputUserData& ud) { \
        is >> is.BEGIN_BRACKET; \
        ObjectTypeAndID array = readArray(is, ud); \
        ud.add("set" #PROP, &array); \
        is >> is.END_BRACKET; \
        return true; \
    }

ADD_ARRAYDATA_FUNCTIONS( VertexData, VertexArray )
ADD_ARRAYDATA_FUNCTIONS( NormalData, NormalArray )
ADD_ARRAYDATA_FUNCTIONS( ColorData, ColorArray )
ADD_ARRAYDATA_FUNCTIONS( SecondaryColorData, SecondaryColorArray )
ADD_ARRAYDATA_FUNCTIONS( FogCoordData, FogCoordArray )

#define ADD_ARRAYLIST_FUNCTIONS( ORIGINAL_PROP, PROP, LISTNAME ) \
    static bool read##ORIGINAL_PROP(InputStream& is, InputUserData& ud) { \
        unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET; \
        for ( unsigned int i=0; i<size; ++i ) { \
            is >> is.PROPERTY("Data") >> is.BEGIN_BRACKET; \
            ObjectTypeAndID array = readArray(is, ud); \
            ud.add("set" #PROP, i, &array); \
            is >> is.END_BRACKET; } \
        is >> is.END_BRACKET; \
        return true; \
    }

ADD_ARRAYLIST_FUNCTIONS( TexCoordData, TexCoordArray, TexCoordArrayList )
ADD_ARRAYLIST_FUNCTIONS( VertexAttribData, VertexAttribArray, VertexAttribArrayList )

// implement backwards compatibility with reading/writing the FastPathHint
static bool readFastPathHint(InputStream& is, InputUserData& ud)
{
    // Compatibility info:
    //   Previous Geometry wrapper (before 3.1.8) require a bool fast-path serializer.
    //   It saves "FastPathHint true" in ascii mode and a single [bool] in binary mode.
    //   Becoming a user serializer, the serializer will first read the name "FastPathHint"
    //   or a [bool] in the checking process, then call the reading function as here. So,
    //   we will only need to read one more bool variable in ascii mode; otherwise do nothing
    bool value = false;
    if ( !is.isBinary() ) is >> value;
    return true;
}

REGISTER_OBJECT_WRAPPER( Geometry,
                         new osg::Geometry,
                         osg::Geometry,
                         "osg::Object osg::Node osg::Drawable osg::Geometry" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::Node")
    }
    //ADD_LIST_SERIALIZER( PrimitiveSetList, osg::Geometry::PrimitiveSetList );  // _primitives
    //ADD_VECTOR_SERIALIZER( PrimitiveSetList, osg::Geometry::PrimitiveSetList, osgDB::BaseSerializer::RW_OBJECT, 0 );
    ADD_OBJECT_LIST_SERIALIZER( PrimitiveSetList, osg::PrimitiveSet );

    ADD_USER_SERIALIZER( VertexData );  // _vertexData
    ADD_USER_SERIALIZER( NormalData );  // _normalData
    ADD_USER_SERIALIZER( ColorData );  // _colorData
    ADD_USER_SERIALIZER( SecondaryColorData );  // _secondaryColorData
    ADD_USER_SERIALIZER( FogCoordData );  // _fogCoordData
    ADD_USER_SERIALIZER( TexCoordData );  // _texCoordList
    ADD_USER_SERIALIZER( VertexAttribData );  // _vertexAttribList
    ADD_USER_SERIALIZER( FastPathHint );  // _fastPathHint

    {
        UPDATE_TO_VERSION_SCOPED( 112 )
        REMOVE_SERIALIZER( VertexData );
        REMOVE_SERIALIZER( NormalData );
        REMOVE_SERIALIZER( ColorData );
        REMOVE_SERIALIZER( SecondaryColorData );
        REMOVE_SERIALIZER( FogCoordData );
        REMOVE_SERIALIZER( TexCoordData );
        REMOVE_SERIALIZER( VertexAttribData );
        REMOVE_SERIALIZER( FastPathHint );

        ADD_OBJECT_SERIALIZER( VertexArray, osg::Array, NULL );
        ADD_OBJECT_SERIALIZER( NormalArray, osg::Array, NULL );
        ADD_OBJECT_SERIALIZER( ColorArray, osg::Array, NULL );
        ADD_OBJECT_SERIALIZER( SecondaryColorArray, osg::Array, NULL );
        ADD_OBJECT_SERIALIZER( FogCoordArray, osg::Array, NULL );

        //ADD_VECTOR_SERIALIZER( TexCoordArrayList, osg::Geometry::ArrayList, osgDB::BaseSerializer::RW_OBJECT, 0 );
        //ADD_VECTOR_SERIALIZER( VertexAttribArrayList, osg::Geometry::ArrayList, osgDB::BaseSerializer::RW_OBJECT, 0 );
        ADD_OBJECT_LIST_SERIALIZER( TexCoordArrayList, osg::Array );
        ADD_OBJECT_LIST_SERIALIZER( VertexAttribArrayList, osg::Array );
    }
}
