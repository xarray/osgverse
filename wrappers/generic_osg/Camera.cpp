#include <GenericReserializer.h>
using namespace osgVerse;

#ifndef GL_VERSION_1_0
#define GL_NONE                           0
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000
#endif

#ifndef GL_ACCUM_BUFFER_BIT
#define GL_ACCUM_BUFFER_BIT 0x00000200
#endif

enum osg_Camera_RenderOrder { PRE_RENDER, NESTED_RENDER, POST_RENDER };
BEGIN_USER_TABLE(RenderOrder, osg_Camera);
    ADD_USER_VALUE(PRE_RENDER);
    ADD_USER_VALUE(NESTED_RENDER);
    ADD_USER_VALUE(POST_RENDER);
END_USER_TABLE()
USER_READ_FUNC(RenderOrder, readOrderValue)

enum osg_Camera_BufferComponent
{
    DEPTH_BUFFER, STENCIL_BUFFER, PACKED_DEPTH_STENCIL_BUFFER, COLOR_BUFFER,
    COLOR_BUFFER0, COLOR_BUFFER1, COLOR_BUFFER2, COLOR_BUFFER3, COLOR_BUFFER4,
    COLOR_BUFFER5, COLOR_BUFFER6, COLOR_BUFFER7, COLOR_BUFFER8, COLOR_BUFFER9,
    COLOR_BUFFER10, COLOR_BUFFER11, COLOR_BUFFER12, COLOR_BUFFER13, COLOR_BUFFER14, COLOR_BUFFER15
};

BEGIN_USER_TABLE(BufferComponent, osg_Camera);
    ADD_USER_VALUE(DEPTH_BUFFER);
    ADD_USER_VALUE(STENCIL_BUFFER);
    ADD_USER_VALUE(PACKED_DEPTH_STENCIL_BUFFER);
    ADD_USER_VALUE(COLOR_BUFFER);
    ADD_USER_VALUE(COLOR_BUFFER0);
    ADD_USER_VALUE(COLOR_BUFFER1);
    ADD_USER_VALUE(COLOR_BUFFER2);
    ADD_USER_VALUE(COLOR_BUFFER3);
    ADD_USER_VALUE(COLOR_BUFFER4);
    ADD_USER_VALUE(COLOR_BUFFER5);
    ADD_USER_VALUE(COLOR_BUFFER6);
    ADD_USER_VALUE(COLOR_BUFFER7);
    ADD_USER_VALUE(COLOR_BUFFER8);
    ADD_USER_VALUE(COLOR_BUFFER9);
    ADD_USER_VALUE(COLOR_BUFFER10);
    ADD_USER_VALUE(COLOR_BUFFER11);
    ADD_USER_VALUE(COLOR_BUFFER12);
    ADD_USER_VALUE(COLOR_BUFFER13);
    ADD_USER_VALUE(COLOR_BUFFER14);
    ADD_USER_VALUE(COLOR_BUFFER15);
END_USER_TABLE()
USER_READ_FUNC(BufferComponent, readBufferComponent)

struct osg_Camera_Attachment
{
    GLenum              _internalFormat;
    ObjectTypeAndID     _image;
    ObjectTypeAndID     _texture;
    unsigned int        _level;
    unsigned int        _face;
    bool                _mipMapGeneration;
    unsigned int        _multisampleSamples;
    unsigned int        _multisampleColorSamples;

    osg_Camera_Attachment() = default;
    ~osg_Camera_Attachment() = default;
};

static osg_Camera_Attachment readBufferAttachment(InputStream& is, InputUserData& ud)
{
    osg_Camera_Attachment attachment;
    char type = -1; is >> is.PROPERTY("Type") >> type;
    if (type == 0)
    {
        is >> is.PROPERTY("InternalFormat") >> attachment._internalFormat;
        return attachment;
    }
    else if (type == 1)
    {
        is >> is.PROPERTY("Image");
        attachment._image = ud.readObjectFromStream(is, "osg::Image");
    }
    else if (type == 2)
    {
        is >> is.PROPERTY("Texture");
        attachment._texture = ud.readObjectFromStream(is, "osg::Texture");
        is >> is.PROPERTY("Level") >> attachment._level;
        is >> is.PROPERTY("Face") >> attachment._face;
        is >> is.PROPERTY("MipMapGeneration") >> attachment._mipMapGeneration;
    }
    else
        return attachment;

    is >> is.PROPERTY("MultisampleSamples") >> attachment._multisampleSamples;
    is >> is.PROPERTY("MultisampleColorSamples") >> attachment._multisampleColorSamples;
    return attachment;
}

// _renderOrder & _renderOrderNum
static bool readRenderOrder(InputStream& is, InputUserData& ud)
{
    int order = readOrderValue(is);
    int orderNumber = 0; is >> orderNumber;
    ud.add("setRenderOrder", order, orderNumber);
    return true;
}

// _bufferAttachmentMap
static bool readBufferAttachmentMap(InputStream& is, InputUserData& ud)
{
    unsigned int size = is.readSize(); is >> is.BEGIN_BRACKET;
    for (unsigned int i = 0; i < size; ++i)
    {
        is >> is.PROPERTY("Attachment");
        osg_Camera_BufferComponent bufferComponent =
            static_cast<osg_Camera_BufferComponent>(readBufferComponent(is));
        is >> is.BEGIN_BRACKET;
        osg_Camera_Attachment attachment = readBufferAttachment(is, ud);
        is >> is.END_BRACKET;

        if (attachment._internalFormat != GL_NONE)
        {
            ud.add("attach", bufferComponent, attachment._internalFormat);
        }
        else if (attachment._image.valid())
        {
            ud.add("attach", bufferComponent, &attachment._image,
                attachment._multisampleSamples, attachment._multisampleColorSamples);
        }
        else if (attachment._texture.valid())
        {
            ud.add("attach", bufferComponent, &attachment._texture,
                attachment._level, attachment._face, attachment._mipMapGeneration,
                attachment._multisampleSamples, attachment._multisampleColorSamples);
        }
    }
    is >> is.END_BRACKET;
    return true;
}

enum osg_Camera_InheritanceMask
{
    COMPUTE_NEAR_FAR_MODE = (0x1 << 0),
    CULLING_MODE = (0x1 << 1),
    LOD_SCALE = (0x1 << 2),
    SMALL_FEATURE_CULLING_PIXEL_SIZE = (0x1 << 3),
    CLAMP_PROJECTION_MATRIX_CALLBACK = (0x1 << 4),
    NEAR_FAR_RATIO = (0x1 << 5),
    IMPOSTOR_ACTIVE = (0x1 << 6),
    DEPTH_SORT_IMPOSTOR_SPRITES = (0x1 << 7),
    IMPOSTOR_PIXEL_ERROR_THRESHOLD = (0x1 << 8),
    NUM_FRAMES_TO_KEEP_IMPOSTORS_SPRITES = (0x1 << 9),
    CULL_MASK = (0x1 << 10),
    CULL_MASK_LEFT = (0x1 << 11),
    CULL_MASK_RIGHT = (0x1 << 12),
    CLEAR_COLOR = (0x1 << 13),
    CLEAR_MASK = (0x1 << 14),
    LIGHTING_MODE = (0x1 << 15),
    LIGHT = (0x1 << 16),
    DRAW_BUFFER = (0x1 << 17),
    READ_BUFFER = (0x1 << 18),

    NO_VARIABLES = 0x00000000,
    ALL_VARIABLES = 0x7FFFFFFF
};

enum osg_Camera_ImplicitBufferMask
{
    IMPLICIT_DEPTH_BUFFER_ATTACHMENT = (1 << 0),
    IMPLICIT_STENCIL_BUFFER_ATTACHMENT = (1 << 1),
    IMPLICIT_COLOR_BUFFER_ATTACHMENT = (1 << 2),
    USE_DISPLAY_SETTINGS_MASK = (~0)
};

REGISTER_OBJECT_WRAPPER(Camera,
                        new osg::Camera,
                        osg::Camera,
                        "osg::Object osg::Node osg::Group osg::Transform osg::Camera")
{
    ADD_BOOL_SERIALIZER(AllowEventFocus, true);  // _allowEventFocus
    BEGIN_BITFLAGS_SERIALIZER(ClearMask, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ADD_BITFLAG_VALUE(COLOR, GL_COLOR_BUFFER_BIT);
        ADD_BITFLAG_VALUE(DEPTH, GL_DEPTH_BUFFER_BIT);
        ADD_BITFLAG_VALUE(ACCUM, GL_ACCUM_BUFFER_BIT);
        ADD_BITFLAG_VALUE(STENCIL, GL_STENCIL_BUFFER_BIT);
    END_BITFLAGS_SERIALIZER();
    ADD_VEC4_SERIALIZER(ClearColor, osg::Vec4());  // _clearColor
    ADD_VEC4_SERIALIZER(ClearAccum, osg::Vec4());  // _clearAccum
    ADD_DOUBLE_SERIALIZER(ClearDepth, 1.0);  // _clearDepth
    ADD_INT_SERIALIZER(ClearStencil, 0);  // _clearStencil
    ADD_OBJECT_SERIALIZER(ColorMask, osg::ColorMask, NULL);  // _colorMask
    ADD_OBJECT_SERIALIZER(Viewport, osg::Viewport, NULL);  // _viewport

    BEGIN_ENUM_SERIALIZER(TransformOrder, PRE_MULTIPLY);
        ADD_ENUM_VALUE(PRE_MULTIPLY);
        ADD_ENUM_VALUE(POST_MULTIPLY);
    END_ENUM_SERIALIZER();  // _transformOrder

    BEGIN_ENUM_SERIALIZER(ProjectionResizePolicy, HORIZONTAL);
        ADD_ENUM_VALUE(FIXED);
        ADD_ENUM_VALUE(HORIZONTAL);
        ADD_ENUM_VALUE(VERTICAL);
    END_ENUM_SERIALIZER();  // _projectionResizePolicy

    ADD_MATRIXD_SERIALIZER(ProjectionMatrix, osg::Matrixd());  // _projectionMatrix
    ADD_MATRIXD_SERIALIZER(ViewMatrix, osg::Matrixd());  // _viewMatrix
    ADD_USER_SERIALIZER(RenderOrder);  // _renderOrder & _renderOrderNum
    ADD_GLENUM_SERIALIZER(DrawBuffer, GLenum, GL_NONE);  // _drawBuffer
    ADD_GLENUM_SERIALIZER(ReadBuffer, GLenum, GL_NONE);  // _readBuffer

    BEGIN_ENUM_SERIALIZER(RenderTargetImplementation, FRAME_BUFFER);
        ADD_ENUM_VALUE(FRAME_BUFFER_OBJECT);
        ADD_ENUM_VALUE(PIXEL_BUFFER_RTT);
        ADD_ENUM_VALUE(PIXEL_BUFFER);
        ADD_ENUM_VALUE(FRAME_BUFFER);
        ADD_ENUM_VALUE(SEPARATE_WINDOW);
    END_ENUM_SERIALIZER();  // _renderTargetImplementation

    ADD_USER_SERIALIZER(BufferAttachmentMap);  // _bufferAttachmentMap
    ADD_OBJECT_SERIALIZER(InitialDrawCallback, osg_Camera::DrawCallback, NULL);  // _initialDrawCallback
    ADD_OBJECT_SERIALIZER(PreDrawCallback, osg_Camera::DrawCallback, NULL);  // _preDrawCallback
    ADD_OBJECT_SERIALIZER(PostDrawCallback, osg_Camera::DrawCallback, NULL);  // _postDrawCallback
    ADD_OBJECT_SERIALIZER(FinalDrawCallback, osg_Camera::DrawCallback, NULL);  // _finalDrawCallback

    {
        UPDATE_TO_VERSION_SCOPED(123)
        BEGIN_ENUM_SERIALIZER(InheritanceMaskActionOnAttributeSetting, DISABLE_ASSOCIATED_INHERITANCE_MASK_BIT);
            ADD_ENUM_VALUE(DISABLE_ASSOCIATED_INHERITANCE_MASK_BIT);
            ADD_ENUM_VALUE(DO_NOT_MODIFY_INHERITANCE_MASK);
        END_ENUM_SERIALIZER();

        BEGIN_INT_BITFLAGS_SERIALIZER(InheritanceMask, osg_Camera_InheritanceMask::ALL_VARIABLES);
            ADD_BITFLAG_VALUE(COMPUTE_NEAR_FAR_MODE, osg_Camera_InheritanceMask::COMPUTE_NEAR_FAR_MODE);
            ADD_BITFLAG_VALUE(CULLING_MODE, osg_Camera_InheritanceMask::CULLING_MODE);
            ADD_BITFLAG_VALUE(LOD_SCALE, osg_Camera_InheritanceMask::LOD_SCALE);
            ADD_BITFLAG_VALUE(SMALL_FEATURE_CULLING_PIXEL_SIZE, osg_Camera_InheritanceMask::SMALL_FEATURE_CULLING_PIXEL_SIZE);
            ADD_BITFLAG_VALUE(CLAMP_PROJECTION_MATRIX_CALLBACK, osg_Camera_InheritanceMask::CLAMP_PROJECTION_MATRIX_CALLBACK);
            ADD_BITFLAG_VALUE(NEAR_FAR_RATIO, osg_Camera_InheritanceMask::NEAR_FAR_RATIO);
            ADD_BITFLAG_VALUE(IMPOSTOR_ACTIVE, osg_Camera_InheritanceMask::IMPOSTOR_ACTIVE);
            ADD_BITFLAG_VALUE(DEPTH_SORT_IMPOSTOR_SPRITES, osg_Camera_InheritanceMask::DEPTH_SORT_IMPOSTOR_SPRITES);
            ADD_BITFLAG_VALUE(IMPOSTOR_PIXEL_ERROR_THRESHOLD, osg_Camera_InheritanceMask::IMPOSTOR_PIXEL_ERROR_THRESHOLD);
            ADD_BITFLAG_VALUE(NUM_FRAMES_TO_KEEP_IMPOSTORS_SPRITES, osg_Camera_InheritanceMask::NUM_FRAMES_TO_KEEP_IMPOSTORS_SPRITES);
            ADD_BITFLAG_VALUE(CULL_MASK, osg_Camera_InheritanceMask::CULL_MASK);
            ADD_BITFLAG_VALUE(CULL_MASK_LEFT, osg_Camera_InheritanceMask::CULL_MASK_LEFT);
            ADD_BITFLAG_VALUE(CULL_MASK_RIGHT, osg_Camera_InheritanceMask::CULL_MASK_RIGHT);
            ADD_BITFLAG_VALUE(CLEAR_COLOR, osg_Camera_InheritanceMask::CLEAR_COLOR);
            ADD_BITFLAG_VALUE(CLEAR_MASK, osg_Camera_InheritanceMask::CLEAR_MASK);
            ADD_BITFLAG_VALUE(LIGHTING_MODE, osg_Camera_InheritanceMask::LIGHTING_MODE);
            ADD_BITFLAG_VALUE(LIGHT, osg_Camera_InheritanceMask::LIGHT);
            ADD_BITFLAG_VALUE(DRAW_BUFFER, osg_Camera_InheritanceMask::DRAW_BUFFER);
            ADD_BITFLAG_VALUE(READ_BUFFER, osg_Camera_InheritanceMask::READ_BUFFER);
            ADD_BITFLAG_VALUE(NO_VARIABLES, osg_Camera_InheritanceMask::NO_VARIABLES);
            /** ADD_BITFLAG_VALUE(ALL_VARIABLES, osg_Camera_InheritanceMask::ALL_VARIABLES);*/
        END_BITFLAGS_SERIALIZER();
    }

    {
        UPDATE_TO_VERSION_SCOPED(140)

        BEGIN_INT_BITFLAGS_SERIALIZER(ImplicitBufferAttachmentRenderMask, osg_Camera_ImplicitBufferMask::USE_DISPLAY_SETTINGS_MASK);
            ADD_BITFLAG_VALUE(IMPLICIT_DEPTH_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_DEPTH_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(IMPLICIT_STENCIL_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_STENCIL_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(IMPLICIT_COLOR_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_COLOR_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(USE_DISPLAY_SETTINGS_MASK, osg_Camera_ImplicitBufferMask::USE_DISPLAY_SETTINGS_MASK);
        END_BITFLAGS_SERIALIZER();

        BEGIN_INT_BITFLAGS_SERIALIZER(ImplicitBufferAttachmentResolveMask, osg_Camera_ImplicitBufferMask::USE_DISPLAY_SETTINGS_MASK);
            ADD_BITFLAG_VALUE(IMPLICIT_DEPTH_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_DEPTH_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(IMPLICIT_STENCIL_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_STENCIL_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(IMPLICIT_COLOR_BUFFER_ATTACHMENT, osg_Camera_ImplicitBufferMask::IMPLICIT_COLOR_BUFFER_ATTACHMENT);
            ADD_BITFLAG_VALUE(USE_DISPLAY_SETTINGS_MASK, osg_Camera_ImplicitBufferMask::USE_DISPLAY_SETTINGS_MASK);
        END_BITFLAGS_SERIALIZER();
    }
}
