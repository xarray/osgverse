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
#include <iostream>
#include <sstream>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(
        argc > 1 ? argv[1] : BASE_DIR "/models/Sponza/Sponza.gltf");
    if (!scene) { OSG_WARN << "Failed to load GLTF model"; return 1; }

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    scene->accept(tsv);

    // Add normal-map & specular-map
    osgVerse::NormalMapGenerator nmg;
    nmg.setTextureUnits(1, 2);
    scene->accept(nmg);

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());
    //sceneRoot->setMatrix(osg::Matrix::rotate(osg::PI_2, osg::X_AXIS));

    // Global shading variables
    osg::Vec3 metallicRoughnessAmbient(0.2f, 0.2f, 0.1f);
    osg::Vec4 lightDir(1.0f, 1.0f, -1.0f, 0.0f), lightDir2(-1.0f, 1.0f, -0.5f, 0.0f);
    osg::Vec4 lightColor(4.0f, 4.0f, 3.8f, 1.0f), lightColor2(1.0f, 1.0f, 1.3f, 1.0f);
    lightDir.normalize(); lightDir2.normalize();

    // Global shading
    osg::StateSet* ss = sceneRoot->getOrCreateStateSet();
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "single_pass.vert.glsl"));
        program->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "single_pass.frag.glsl"));
        program->addBindAttribLocation(osgVerse::attributeNames[6], 6);
        program->addBindAttribLocation(osgVerse::attributeNames[7], 7);

        ss->setAttributeAndModes(program.get());
        ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(1, osgVerse::createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f)));
        ss->setTextureAttributeAndModes(2, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(5, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(6, osgVerse::createTexture2D(
            osgDB::readImageFile(BASE_DIR "/skyboxes/barcelona.hdr")));
        ss->addUniform(new osg::Uniform("DiffuseMap", (int)0));
        ss->addUniform(new osg::Uniform("NormalMap", (int)1));
        ss->addUniform(new osg::Uniform("SpecularMap", (int)2));
        ss->addUniform(new osg::Uniform("EmissiveMap", (int)5));
        ss->addUniform(new osg::Uniform("ReflectionMap", (int)6));

        ss->getOrCreateUniform("dLightColor", osg::Uniform::FLOAT_VEC4)->set(lightColor);
        ss->getOrCreateUniform("dLightColor2", osg::Uniform::FLOAT_VEC4)->set(lightColor2);
        ss->getOrCreateUniform("metallicRoughness", osg::Uniform::FLOAT_VEC3)->set(metallicRoughnessAmbient);
    }

    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.15,15,1.scale.0,0,-300.trans");
    //osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile("lz.osgt.0,0,-250.trans");

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    //root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        osg::Matrixf viewMatrix = viewer.getCamera()->getViewMatrix();
        ss->getOrCreateUniform("dLightDirection", osg::Uniform::FLOAT_VEC4)->set(lightDir * viewMatrix);
        ss->getOrCreateUniform("dLightDirection2", osg::Uniform::FLOAT_VEC4)->set(lightDir2 * viewMatrix);
        viewer.frame();
    }
    return 0;
}
