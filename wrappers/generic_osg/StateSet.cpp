#include <GenericReserializer.h>
using namespace osgVerse;

enum osg_StateAttribute
{
    /** means that associated GLMode and Override is disabled.*/
    OFF = 0x0,
    /** means that associated GLMode is enabled and Override is disabled.*/
    ON = 0x1,
    /** Overriding of GLMode's or StateAttributes is enabled, so that state below it is overridden.*/
    OVERRIDE = 0x2,
    /** Protecting of GLMode's or StateAttributes is enabled, so that state from above cannot override this and below state.*/
    PROTECTED = 0x4,
    /** means that GLMode or StateAttribute should be inherited from above.*/
    INHERIT = 0x8
};

static int readValue(InputStream& is, InputUserData& ud)
{
    int value = 0;
    if ( is.isBinary() )
        is >> value;
    else
    {
        std::string enumValue;
        is >> enumValue;

        if (enumValue.find("OFF")!=std::string::npos) value = osg_StateAttribute::OFF;
        if (enumValue.find("ON")!=std::string::npos) value = osg_StateAttribute::ON;
        if (enumValue.find("OVERRIDE")!=std::string::npos) value = value | osg_StateAttribute::OVERRIDE;
        if (enumValue.find("PROTECTED")!=std::string::npos) value = value | osg_StateAttribute::PROTECTED;
        if (enumValue.find("INHERIT")!=std::string::npos) value = value | osg_StateAttribute::INHERIT;
    }
    return value;
}

//// TODO!!

/*static void readModes(InputStream& is, std::map<GLenum, unsigned int>& modes)
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            DEF_GLENUM(mode); is >> mode;
            int value = readValue( is );
            modes[mode.get()] = value;
        }
        is >> is.END_BRACKET;
    }
}

static void readAttributes(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize();
    if ( size>0 )
    {
        is >> is.BEGIN_BRACKET;
        for ( unsigned int i=0; i<size; ++i )
        {
            ObjectTypeAndID indices = ud.readObjectFromStream(is, "osg::StateAttribute");
            is >> is.PROPERTY("Value");
            int value = readValue( is );
            if ( sa )
                attrs[sa->getTypeMemberPair()] = osg::StateSet::RefAttributePair(sa,value);
        }
        is >> is.END_BRACKET;
    }
}*/

// _modeList
/*static bool readModeList(InputStream& is, InputUserData& ud)
{
    std::map<GLenum, unsigned int> modes; readModes( is, modes );
    for ( osg::StateSet::ModeList::iterator itr=modes.begin();
          itr!=modes.end(); ++itr )
    {
        ss.setMode( itr->first, itr->second );
    }
    return true;
}

// _attributeList
static bool readAttributeList(InputStream& is, InputUserData& ud)
{
    std::map<std::pair<Type, unsigned int>, RefAttributePair> attrs; readAttributes( is, attrs );
    for ( osg::StateSet::AttributeList::iterator itr=attrs.begin();
          itr!=attrs.end(); ++itr )
    {
        ss.setAttribute( itr->second.first, itr->second.second );
    }
    return true;
}

// _textureModeList
static bool readTextureModeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    osg::StateSet::ModeList modes;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Data");
        readModes( is, modes );
        for ( osg::StateSet::ModeList::iterator itr=modes.begin();
              itr!=modes.end(); ++itr )
        {
            ss.setTextureMode( i, itr->first, itr->second );
        }
        modes.clear();
    }
    is >> is.END_BRACKET;
    return true;
}

// _textureAttributeList
static bool readTextureAttributeList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    osg::StateSet::AttributeList attrs;
    for ( unsigned int i=0; i<size; ++i )
    {
        is >> is.PROPERTY("Data");
        readAttributes( is, attrs );
        for ( osg::StateSet::AttributeList::iterator itr=attrs.begin();
              itr!=attrs.end(); ++itr )
        {
            ss.setTextureAttribute( i, itr->second.first, itr->second.second );
        }
        attrs.clear();
    }
    is >> is.END_BRACKET;
    return true;
}*/

// _uniformList
static bool readUniformList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        ObjectTypeAndID uniform = ud.readObjectFromStream(is, "osg::UniformBase");
        is >> is.PROPERTY("Value");
        int value = readValue( is, ud );
        if ( uniform.valid() )
            ud.add("addUniform", uniform, value);
    }
    is >> is.END_BRACKET;
    return true;
}

// _defineList
static bool readDefineList(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for ( unsigned int i=0; i<size; ++i )
    {
        std::string defineName;
        is.readWrappedString( defineName );

        std::string defineValue;
        is.readWrappedString( defineValue );

        is >> is.PROPERTY("Value");
        int overrideValue = readValue( is, ud );

        ud.add("setDefine", defineName, defineValue, overrideValue);
    }
    is >> is.END_BRACKET;
    return true;
}

REGISTER_OBJECT_WRAPPER( StateSet,
                         new osg::StateSet,
                         osg::StateSet,
                         "osg::Object osg::StateSet" )
{
    //ADD_USER_SERIALIZER( ModeList );  // _modeList
    //ADD_USER_SERIALIZER( AttributeList );  // _attributeList
    //ADD_USER_SERIALIZER( TextureModeList );  // _textureModeList
    //ADD_USER_SERIALIZER( TextureAttributeList );  // _textureAttributeList
    ADD_USER_SERIALIZER( UniformList );  // _uniformList
    ADD_INT_SERIALIZER( RenderingHint, osg::StateSet::DEFAULT_BIN );  // _renderingHint

    BEGIN_ENUM_SERIALIZER( RenderBinMode, INHERIT_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE( INHERIT_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE( USE_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE( OVERRIDE_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE( PROTECTED_RENDERBIN_DETAILS );
        ADD_ENUM_VALUE( OVERRIDE_PROTECTED_RENDERBIN_DETAILS );
    END_ENUM_SERIALIZER();  // _binMode

    ADD_INT_SERIALIZER( BinNumber, 0 );  // _binNum
    ADD_STRING_SERIALIZER( BinName, "" );  // _binName
    ADD_BOOL_SERIALIZER( NestRenderBins, true );  // _nestRenderBins
    ADD_OBJECT_SERIALIZER( UpdateCallback, osg::StateSet::Callback, NULL );  // _updateCallback
    ADD_OBJECT_SERIALIZER( EventCallback, osg::StateSet::Callback, NULL );  // _eventCallback

    {
        UPDATE_TO_VERSION_SCOPED( 151 )
        ADD_USER_SERIALIZER( DefineList );  // _defineList
    }
}
