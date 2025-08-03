#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Depth>
#include <osg/CullFace>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgGA/EventVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <readerwriter/EarthManipulator.h>
#include <pipeline/Pipeline.h>
#include <VerseCommon.h>
#include <iostream>
#include <sstream>

const char* innerVertCode = {
    "uniform mat4 osg_ViewMatrix, osg_ViewMatrixInverse; \n"
    "VERSE_VS_OUT vec3 normalInWorld; \n"
    "VERSE_VS_OUT vec3 vertexInWorld; \n"
    "VERSE_VS_OUT vec4 texCoord, color; \n"

    "void main() {\n"
    "    mat4 modelMatrix = osg_ViewMatrixInverse * VERSE_MATRIX_MV; \n"
    "    vertexInWorld = vec3(modelMatrix * osg_Vertex); \n"
    "    normalInWorld = normalize(vec3(osg_ViewMatrixInverse * vec4(VERSE_MATRIX_N * gl_Normal, 0.0))); \n"
    "    texCoord = osg_MultiTexCoord0; color = osg_Color; \n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex; \n"
    "}\n"
};

const char* innerFragCode = {
    "VERSE_FS_IN vec3 normalInWorld; \n"
    "VERSE_FS_IN vec3 vertexInWorld; \n"
    "VERSE_FS_IN vec4 texCoord, color; \n"

    "#ifdef VERSE_GLES3\n"
    "layout(location = 0) VERSE_FS_OUT vec4 fragColor;\n"
    "layout(location = 1) VERSE_FS_OUT vec4 fragOrigin;\n"
    "#endif\n"

    "void main() {\n"
    "    vec4 finalColor = vec4(1.0, 1.0, 0.0, 1.0); \n"
    "#ifdef VERSE_GLES3\n"
    "    fragColor = finalColor; \n"
    "    fragOrigin = vec4(1.0); \n"
    "#else\n"
    "    gl_FragData[0] = finalColor; \n"
    "    gl_FragData[1] = vec4(1.0); \n"
    "#endif\n"
    "}\n"
};

osg::Node* configureInternal(osgViewer::View& viewer, osg::Node* earth, unsigned int mask)
{
    osg::Shader* vs = new osg::Shader(osg::Shader::VERTEX, innerVertCode);
    osg::Shader* fs = new osg::Shader(osg::Shader::FRAGMENT, innerFragCode);
    osg::ref_ptr<osg::Program> program = new osg::Program;
    vs->setName("Inner_VS"); program->addShader(vs);
    fs->setName("Inner_FS"); program->addShader(fs);
    osgVerse::Pipeline::createShaderDefinitions(vs, 100, 130);
    osgVerse::Pipeline::createShaderDefinitions(fs, 100, 130);  // FIXME

    osg::ref_ptr<osg::Geode> innerRoot = new osg::Geode;
    //innerRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    innerRoot->getOrCreateStateSet()->setAttributeAndModes(program.get());
    innerRoot->setNodeMask(mask);

    double d = osg::WGS_84_RADIUS_EQUATOR * 0.5;
    innerRoot->addDrawable(new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(), d)));

    //viewer.addEventHandler(new InternalHandler(clip0.get(), clip1.get(), clip2.get()));
    return innerRoot.release();
}
