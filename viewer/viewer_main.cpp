#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/StateSetManipulator>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <random>
#include <iostream>
#include <sstream>
#include <readerwriter/LoadSceneGLTF.h>
#include <pipeline/ShadowModule.h>
#include <pipeline/Utilities.h>

#define SHADER_DIR "../shaders/"
#define FORWARD_SCENE_MASK  0x0000ff00
#define DEFERRED_SCENE_MASK 0x00ff0000
#define SHADOW_CASTER_MASK  0x01000000

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

static std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
static std::default_random_engine generator;

static osg::Uniform* generateSsaoSamples(int numberOfSamples)
{
    auto lerp = [](float a, float b, float f) -> float { return a + f * (b - a); };
    osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform(
        osg::Uniform::FLOAT_VEC3, "ssaoSamples", numberOfSamples);
    for (int i = 0; i < numberOfSamples; ++i)
    {
        osg::Vec3 sample(randomFloats(generator) * 2.0f - 1.0f,
                         randomFloats(generator) * 2.0f - 1.0f, randomFloats(generator));
        sample.normalize();

        float rand = randomFloats(generator);
        sample[0] *= rand; sample[1] *= rand; sample[2] *= rand;

        float scale = (float)i / (float)numberOfSamples;
        scale = lerp(0.1, 1.0, scale * scale);
        sample[0] *= scale; sample[1] *= scale; sample[2] *= scale;
        uniform->setElement(i, sample);
    }
    return uniform.release();
}

static osg::Uniform* generateSsaoNoises(int numberOfNoise)
{
    osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform(
        osg::Uniform::FLOAT_VEC3, "ssaoNoises", numberOfNoise);
    for (int i = 0; i < numberOfNoise; ++i)
    {
        osg::Vec3 noise(randomFloats(generator) * 2.0f - 1.0f,
                        randomFloats(generator) * 2.0f - 1.0f, 0.0f);
        uniform->setElement(i, noise);
    }
    return uniform.release();
}

static void setupPipeline(osgVerse::Pipeline* p, osgViewer::View* view, osg::Group* root,
                          unsigned int originW, unsigned int originH)
{
    p->startStages(originW, originH);

    osg::ref_ptr<osgVerse::ShadowModule> shadow = new osgVerse::ShadowModule(p, true);
    shadow->setLightState(osg::Vec3(), osg::Vec3(1.0f, 0.0f, -1.0f), 1500.0f);
    shadow->createStages(2048, 1,
        osgDB::readShaderFile(SHADER_DIR "std_shadow_cast.vert"), 
        osgDB::readShaderFile(SHADER_DIR "std_shadow_cast.frag"), SHADOW_CASTER_MASK);
    view->getCamera()->addUpdateCallback(shadow.get());

    osgVerse::Pipeline::Stage* gbuffer = p->addInputStage("GBuffer", DEFERRED_SCENE_MASK,
        osgDB::readShaderFile(SHADER_DIR "std_gbuffer.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_gbuffer.frag"), 5,
        "NormalBuffer", osgVerse::Pipeline::RGBA_INT10_2,
        "DiffuseMetallicBuffer", osgVerse::Pipeline::RGBA_INT8,
        "SpecularRoughnessBuffer", osgVerse::Pipeline::RGBA_INT8,
        "EmissionOcclusionBuffer", osgVerse::Pipeline::RGBA_FLOAT16,
        "DepthBuffer", osgVerse::Pipeline::DEPTH32);

    osgVerse::Pipeline::Stage* brdfLut = p->addDeferredStage("BrdfLut", true,
        osgDB::readShaderFile(SHADER_DIR "std_common_quad.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_brdf_lut.frag"), 1,
        "BrdfLutBuffer", osgVerse::Pipeline::RG_FLOAT16);

    osgVerse::Pipeline::Stage* ssao = p->addWorkStage("Ssao",
        osgDB::readShaderFile(SHADER_DIR "std_common_quad.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_ssao.frag"), 1,
        "SsaoBuffer", osgVerse::Pipeline::R_INT8);
    ssao->applyBuffer(*gbuffer, "NormalBuffer", 0);
    ssao->applyBuffer(*gbuffer, "DepthBuffer", 1);
    ssao->applyUniform(generateSsaoSamples(8));
    ssao->applyUniform(generateSsaoNoises(4));

    osgVerse::Pipeline::Stage* ssaoBlur = p->addWorkStage("SsaoBlur",
        osgDB::readShaderFile(SHADER_DIR "std_common_quad.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_ssao_blur.frag"), 1,
        "SsaoBlurredBuffer", osgVerse::Pipeline::R_INT8);
    ssaoBlur->applyBuffer(*ssao, "SsaoBuffer", 0);

    osgVerse::Pipeline::Stage* lighting = p->addWorkStage("Lighting",
        osgDB::readShaderFile(SHADER_DIR "std_common_quad.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_pbr_lighting.frag"), 1,
        "ColorBuffer", osgVerse::Pipeline::RGB_FLOAT16);
    lighting->applyBuffer(*gbuffer, "NormalBuffer", 0);
    lighting->applyBuffer(*gbuffer, "DiffuseMetallicBuffer", 1);
    lighting->applyBuffer(*gbuffer, "SpecularRoughnessBuffer", 2);
    lighting->applyBuffer(*gbuffer, "EmissionOcclusionBuffer", 3);
    lighting->applyBuffer(*gbuffer, "DepthBuffer", 4);
    lighting->applyBuffer(*brdfLut, "BrdfLutBuffer", 5);
    //lighting->applyTexture(shadow->getTextureArray(), "ShadowMapArray", 5);
    //lighting->applyUniform(shadow->getLightMatrices());

    osgVerse::Pipeline::Stage* output = p->addDisplayStage("Final",
        osgDB::readShaderFile(SHADER_DIR "std_common_quad.vert"),
        osgDB::readShaderFile(SHADER_DIR "std_display.frag"), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
    output->applyBuffer(*lighting, "ColorBuffer", 0);
    output->applyBuffer(*ssaoBlur, "SsaoBlurredBuffer", 1);

    p->applyStagesToView(view, FORWARD_SCENE_MASK);
    p->requireDepthBlit(gbuffer, true);
    
    //shadow->getFrustumGeode()->setNodeMask(FORWARD_SCENE_MASK);
    //root->addChild(shadow->getFrustumGeode());
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene = osgVerse::loadGltf("../models/Sponza/Sponza.gltf", false);
    //osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile("cessna.osg");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    sceneRoot->setNodeMask(DEFERRED_SCENE_MASK | SHADOW_CASTER_MASK);
    sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.0,0,-250.trans");
    otherSceneRoot->setNodeMask(~DEFERRED_SCENE_MASK);

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osgVerse::Pipeline> pipeline = new osgVerse::Pipeline;
    MyViewer viewer(pipeline.get());
    setupPipeline(pipeline.get(), &viewer, root.get(), 1920, 1080);

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    
    //return viewer.run();
    while (!viewer.done())
    {
        viewer.frame();
    }
    return 0;
}
