#include <GenericReserializer.h>
using namespace osgVerse;

// _columns, _rows
static bool readArea(InputStream& is, InputUserData& ud)
{
    unsigned int numCols, numRows;
    is >> numCols >> numRows;
    ud.add("allocate", numCols, numRows);
    return true;
}

// _heights
static bool readHeights(InputStream& is, InputUserData& ud)
{
    ObjectTypeAndID farray = ud.readObjectFromStream(is, "osg::FloatArray");
    if ( farray.valid() )
    {
        unsigned int numCols = 0, numRows = 0; ud.get("allocate", &numCols, &numRows);
        unsigned int fsize = 0; ud.get(farray, "size", &fsize);
        if ( fsize < numRows*numCols ) return false;

        unsigned int index = 0;
        for ( unsigned int r=0; r<numRows; ++r )
        {
            for (unsigned int c = 0; c < numCols; ++c)
            {
                float v = 0.0f; ud.get(farray, "index", &index, &v);
                ud.add("setHeight", c, r, v); index++;
            }
        }
    }
    return true;
}

REGISTER_OBJECT_WRAPPER( HeightField,
                         new osg::HeightField,
                         osg::HeightField,
                         "osg::Object osg::Shape osg::HeightField" )
{
    ADD_USER_SERIALIZER( Area );  // _columns, _rows
    ADD_VEC3_SERIALIZER( Origin, osg::Vec3() );  // _origin
    ADD_FLOAT_SERIALIZER( XInterval, 0.0f );  // _dx
    ADD_FLOAT_SERIALIZER( YInterval, 0.0f );  // _dy
    ADD_FLOAT_SERIALIZER( SkirtHeight, 0.0f );  // _skirtHeight
    ADD_UINT_SERIALIZER( BorderWidth, 0 );  // _borderWidth
    ADD_QUAT_SERIALIZER( Rotation, osg::Quat() );  // _rotation
    ADD_USER_SERIALIZER( Heights );  // _heights
}
