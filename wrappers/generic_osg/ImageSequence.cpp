#include <GenericReserializer.h>
using namespace osgVerse;

// _fileNames
static bool readFileNames(InputStream& is, InputUserData& ud)
{
    unsigned int files = 0; is >> files >> is.BEGIN_BRACKET;
    //if (is.getOptions()) image.setReadOptions(new osgDB::Options(*is.getOptions()));
    for ( unsigned int i=0; i<files; ++i )
    {
        std::string filename; is.readWrappedString( filename );
        ud.add("addImageFile", filename);
    }
    is >> is.END_BRACKET;
    return true;
}

// _images
static bool readImages(InputStream& is, InputUserData& ud)
{
    unsigned int images = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<images; ++i )
    {
        //osg::ref_ptr<osg::Image> img = is.readImage();
        ObjectTypeAndID img = ud.readObjectFromStream(is, "osg::Image");
        if (img.valid()) ud.add("addImage", &img);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( ImageSequence,
                         new osg::ImageSequence,
                         osg::ImageSequence,
                         "osg::Object osg::BufferData osg::Image osg::ImageStream osg::ImageSequence" )
{
    {
         UPDATE_TO_VERSION_SCOPED( 154 )
         ADDED_ASSOCIATE("osg::BufferData")
    }
    ADD_DOUBLE_SERIALIZER( ReferenceTime, DBL_MAX );  // _referenceTime
    ADD_DOUBLE_SERIALIZER( TimeMultiplier, 1.0 );  // _timeMultiplier

    BEGIN_ENUM_SERIALIZER( Mode, PRE_LOAD_ALL_IMAGES );
        ADD_ENUM_VALUE( PRE_LOAD_ALL_IMAGES );
        ADD_ENUM_VALUE( PAGE_AND_RETAIN_IMAGES );
        ADD_ENUM_VALUE( PAGE_AND_DISCARD_USED_IMAGES );
        ADD_ENUM_VALUE( LOAD_AND_DISCARD_IN_UPDATE_TRAVERSAL );
        ADD_ENUM_VALUE( LOAD_AND_RETAIN_IN_UPDATE_TRAVERSAL );
    END_ENUM_SERIALIZER();  // _mode

    ADD_DOUBLE_SERIALIZER( Length, 1.0 );  // _length
    ADD_USER_SERIALIZER( FileNames );  // _fileNames
    ADD_USER_SERIALIZER( Images );  // _images
}
