#include <osg/io_utils>
#include <osg/ComputeBoundsVisitor>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osgDB/FileNameUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>

#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <pipeline/UserInputModule.h>
#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class MyViewer : public osgViewer::Viewer
{
public:
    MyViewer(osgVerse::Pipeline* p) : osgViewer::Viewer(), _pipeline(p) {}
    osg::ref_ptr<osgVerse::Pipeline> _pipeline;

protected:
    virtual osg::GraphicsOperation* createRenderer(osg::Camera* camera)
    {
        if (_pipeline.valid()) return _pipeline->createRenderer(camera);
        else return osgViewer::Viewer::createRenderer(camera);
    }
};

// osgVerse defines some built-in shader variables for compatibility use:
// - matrics: VERSE_MATRIX_MVP, VERSE_MATRIX_MV, VERSE_MATRIX_P, VERSE_MATRIX_N
// - texel retrievers: VERSE_TEX1D, VERSE_TEX2D, VERSE_TEX3D, VERSE_TEXCUBE
// - qualifiers: VERSE_VS_IN, VERSE_VS_OUT, VERSE_FS_IN, VERSE_VS_OUT
// - alternative of gl_FragColor: VERSE_FS_FINAL
// Try to use them instead of GLSL ones to make osgVerse work smoothly!

static const char* middleFragmentShaderCode =
{
    "uniform sampler2D DiffuseMetallicBuffer;\n"
    "VERSE_FS_IN vec4 texCoord0;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    vec2 uv0 = texCoord0.xy;\n"
    "    vec4 color = VERSE_TEX2D(DiffuseMetallicBuffer, uv0);\n"
    "    fragData = color.bgra;\n"  // some very simple trick...
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

static const char* displayFragmentShaderCode =
{
    "uniform sampler2D ColorBuffer;\n"
    "uniform sampler2D DepthBuffer;\n"
    "VERSE_FS_IN vec4 texCoord0;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    vec2 uv0 = texCoord0.xy;\n"
    "    vec4 color = VERSE_TEX2D(ColorBuffer, uv0);\n"
    "    vec4 depth = VERSE_TEX2D(DepthBuffer, uv0);\n"
    "    fragData = gl_FragCoord.y < 500 ? depth : color;\n"
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

static const char* inputFragmentShaderCode =
{
    "uniform sampler2D DiffuseMap;\n"
    "varying vec4 gl_TexCoord[gl_MaxTextureCoords];\n"
    "void main() {\n"
    "    gl_FragColor = texture2D(DiffuseMap, gl_TexCoord[0].xy);\n"
    "}\n"
};

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf.125,125,125.scale");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv; scene->accept(tsv);
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));
    osgVerse::Pipeline::setPipelineMask(*sceneRoot, DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osg.15,15,1.scale.0,0,-300.trans");
    /*osg::ref_ptr<osg::MatrixTransform> otherSceneRoot = new osg::MatrixTransform;
    otherSceneRoot->addChild(scene.get());
    otherSceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS) *
                              osg::Matrix::translate(0.0f, 3000.0f, 0.0f));*/
    if (otherSceneRoot.valid())
    {
        otherSceneRoot->getOrCreateStateSet()->addUniform(new osg::Uniform("DiffuseMap", 0));
        osgVerse::Pipeline::setPipelineMask(*otherSceneRoot, CUSTOM_INPUT_MASK);
    }

    // Post-HUD display
    osg::ref_ptr<osg::Camera> postCamera = new osg::Camera;
    postCamera->setClearMask(GL_DEPTH_BUFFER_BIT);
    postCamera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    postCamera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
    postCamera->setViewMatrix(osg::Matrix::identity());
    postCamera->setRenderOrder(osg::Camera::POST_RENDER, 10000);
    postCamera->setComputeNearFarMode(osg::Camera::DO_NOT_COMPUTE_NEAR_FAR);
    osgVerse::Pipeline::setPipelineMask(*postCamera, FORWARD_SCENE_MASK);

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(sceneRoot.get());
    root->addChild(otherSceneRoot.get());
    
    // Create the pipeline and the viewer
    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    viewer.setUpViewOnSingleScreen(0);
    
    {
        // 1. Checking GL version is always a good start
        osgVerse::GLVersionData* glData = osgVerse::queryOpenGLVersion(pipeline.get(), false);
        if (glData && (!glData->glslSupported || !glData->fboSupported))
        {
            OSG_FATAL << "[SimplePipeline] Necessary OpenGL features missing. The pipeline "
                      << "can not work on your machine at present." << std::endl;
            return false;
        }

        // 2. Start configuring stages: its resolution and graphics context
        pipeline->startStages(1920, 1080, viewer.getCamera()->getGraphicsContext());

        // 3. Add gbuffer stage: this will require the hardware to support MRT
        osgVerse::Pipeline::Stage* gbuffer = pipeline->addInputStage(
            "GBuffer", DEFERRED_SCENE_MASK, 0,
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "std_gbuffer.vert.glsl"),
            osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "std_gbuffer.frag.glsl"), 5,
            "NormalBuffer", osgVerse::Pipeline::RGBA_INT8,
            "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
            "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
            "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
            "DepthBuffer", osgVerse::Pipeline::DEPTH24_STENCIL8);

#if false
        // Optional, add another custom input pass, which may use forward pipeline to render
        // transparent objects and third party nodekits.
        osgVerse::UserInputModule* inModule = new osgVerse::UserInputModule("Forward", pipeline.get());
        {
            osgVerse::Pipeline::Stage* customIn = inModule->createStages(
                CUSTOM_INPUT_MASK, NULL, NULL,//new osg::Shader(osg::Shader::FRAGMENT, inputFragmentShaderCode),
                "ColorBuffer", gbuffer->getBufferTexture("DiffuseMetallicBuffer"),
                "DepthBuffer", gbuffer->getBufferTexture(osg::Camera::DEPTH_BUFFER));
        }
        viewer.getCamera()->addUpdateCallback(inModule);
#endif

        // 4. Add a custom middle stage
        osgVerse::Pipeline::Stage* testStage = pipeline->addWorkStage("TestStage", 1.0f,
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "std_common_quad.vert.glsl"),
            new osg::Shader(osg::Shader::FRAGMENT, middleFragmentShaderCode), 1,
            "MiddleBuffer", osgVerse::Pipeline::RGB_INT8);
        //testStage->applyBuffer("ColorBuffer", 0, pipeline.get());  // get last buffer
        testStage->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 0);

#if true
        // Optional, add another custom input pass, which may use forward pipeline to render
        // transparent objects and third party nodekits.
        osgVerse::UserInputModule* inModule = new osgVerse::UserInputModule("Forward", pipeline.get());
        {
            osgVerse::Pipeline::Stage* customIn = inModule->createStages(
                CUSTOM_INPUT_MASK, NULL, NULL,//new osg::Shader(osg::Shader::FRAGMENT, inputFragmentShaderCode),
                "ColorBuffer", testStage->getBufferTexture("MiddleBuffer"),
                "DepthBuffer", gbuffer->getBufferTexture(osg::Camera::DEPTH_BUFFER));
        }
        viewer.getCamera()->addUpdateCallback(inModule);
#endif

        // 5. Add a custom display stage
        osgVerse::Pipeline::Stage* output = pipeline->addDisplayStage("Final",
            osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "std_common_quad.vert.glsl"),
            new osg::Shader(osg::Shader::FRAGMENT, displayFragmentShaderCode),
            osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
        output->applyBuffer(*testStage, "MiddleBuffer", "ColorBuffer", 0);
        output->applyBuffer(*gbuffer, "DepthBuffer", 1);

        // 6. Apply stages to viewer's slaves, also finish stage configuring
        pipeline->applyStagesToView(&viewer, FORWARD_SCENE_MASK);

        // 7. Add gbuffer stage to depth bliting list
        pipeline->requireDepthBlit(gbuffer, true);
    }

    // Start the viewer
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    // Must use single-threaded if you share buffers between stages!
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    return viewer.run();
}
