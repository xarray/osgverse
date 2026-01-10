#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <readerwriter/Utilities.h>
#include <pipeline/Utilities.h>
#include <pipeline/Global.h>
#include <pipeline/Pipeline.h>
#include <pipeline/ShaderLibrary.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
#endif
USE_GRAPICSWINDOW_IMPLEMENTATION(SDL)
USE_GRAPICSWINDOW_IMPLEMENTATION(GLFW)

const char* vertCode = {
    "VERSE_VS_OUT vec3 viewInEye, normalInEye;\n"
    "VERSE_VS_OUT vec4 vertexColor, texCoord;\n"
    "void main() {\n"
    "    viewInEye = normalize(VERSE_MATRIX_MV * osg_Vertex).xyz;\n"
    "    normalInEye = normalize(VERSE_MATRIX_N * osg_Normal);\n"
    "    vertexColor = osg_Color;\n"
    "    texCoord = osg_MultiTexCoord0;\n"
    "    gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n"
    "}\n"
};

const char* fragCode = {
    "uniform sampler2D DiffuseMap;\n"
    "VERSE_FS_IN vec3 viewInEye, normalInEye;\n"
    "VERSE_FS_IN vec4 vertexColor, texCoord;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    vec3 lightDir = normalize(vec3(0.1, 0.1, 1.0));\n"
    "    vec4 diffuse = vec4(0.6, 0.6, 0.65, 1.0), specular = vec4(1.0, 1.0, 0.96, 1.0);\n"
    "    vec4 color = VERSE_TEX2D(DiffuseMap, texCoord.st);\n"
    "    float dTerm = VERSE_lambertDiffuse(lightDir, normalInEye);\n"
    "    float sTerm = VERSE_blinnPhongSpecular(lightDir, viewInEye, normalInEye, 64.0);\n"
    "    fragData = color * (diffuse * dTerm + specular * sTerm);\n"
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR + "/models/Sponza/Sponza.gltf");
    if (scene.valid())
    {
        // Add tangent/bi-normal arrays for normal mapping
        //osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
        osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

        // Create program for demonstrating shader module uses
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode));
        program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));
        osgVerse::ShaderLibrary::instance()->updateProgram(*program);

        osg::StateSet* stateSet = scene->getOrCreateStateSet();
        stateSet->setAttribute(program.get());
        stateSet->addUniform(new osg::Uniform("DiffuseMap", (int)0));
    }

    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    if (scene.valid()) sceneRoot->addChild(scene.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setRealizeOperation(new osgVerse::RealizeOperation);

    // Create the graphics window
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->x = 50; traits->y = 50; traits->width = 1280; traits->height = 720;
    traits->alpha = 8; traits->depth = 24; traits->stencil = 8;
    traits->windowDecoration = true; traits->doubleBuffer = true;
    traits->readDISPLAY(); traits->setUndefinedScreenDetailsToDefaultScreen();
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    traits->windowingSystemPreference = "SDL";
#endif

    osg::ref_ptr<osg::GraphicsContext> gc = osg::GraphicsContext::createGraphicsContext(traits.get());
    viewer.getCamera()->setGraphicsContext(gc.get());
    viewer.getCamera()->setViewport(0, 0, traits->width, traits->height);
    viewer.getCamera()->setDrawBuffer(GL_BACK);
    viewer.getCamera()->setReadBuffer(GL_BACK);
    viewer.getCamera()->setProjectionMatrixAsPerspective(
        30.0f, static_cast<double>(traits->width) / static_cast<double>(traits->height), 1.0f, 10000.0f);

    // Start the main loop
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
