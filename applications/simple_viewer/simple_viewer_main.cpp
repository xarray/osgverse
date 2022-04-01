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
#include <pipeline/Utilities.h>
#define SHADER_DIR "../shaders/"

osg::Texture2D* createTexture2D(osg::Image* image)
{
    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR_MIPMAP_LINEAR);
    tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
    tex2D->setWrap(osg::Texture2D::WRAP_S, osg::Texture2D::MIRROR);
    tex2D->setWrap(osg::Texture2D::WRAP_T, osg::Texture2D::MIRROR);
    tex2D->setResizeNonPowerOfTwoHint(false);
    tex2D->setImage(image); return tex2D.release();
}

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> grass = osgDB::readNodeFile("../PeiLi/grass.ive");
    osg::ref_ptr<osg::Node> windows = osgDB::readNodeFile("../PeiLi/windows.ive");
    osg::ref_ptr<osg::Node> roads = osgDB::readNodeFile("../PeiLi/roads.ive");
    osg::ref_ptr<osg::Node> ground = osgDB::readNodeFile("../PeiLi/ground.ive");

    grass->getOrCreateStateSet()->getOrCreateUniform(
        "metallicRoughness", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(1.0f, 1.0f, 0.0f));
    windows->getOrCreateStateSet()->getOrCreateUniform(
        "metallicRoughness", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.2f, 0.2f, 0.8f));
    roads->getOrCreateStateSet()->getOrCreateUniform(
        "metallicRoughness", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.5f, 0.4f, 0.2f));
    ground->getOrCreateStateSet()->getOrCreateUniform(
        "metallicRoughness", osg::Uniform::FLOAT_VEC3)->set(osg::Vec3(0.5f, 0.8f, 0.2f));

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(grass.get());
    root->addChild(windows.get());
    root->addChild(roads.get());
    root->addChild(ground.get());

    // Add tangent/bi-normal arrays for normal mapping
    osgVerse::TangentSpaceVisitor tsv;
    root->accept(tsv);

    // Add normal-map & specular-map
    osgVerse::NormalMapGenerator nmg;
    nmg.setTextureUnits(2, 3);
    root->accept(nmg);  // TODO

    // Global shading
    osg::StateSet* ss = root->getOrCreateStateSet();
    {
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, "../PeiLi/global.vert"));
        program->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, "../PeiLi/global.frag"));
        program->addBindAttribLocation("osg_Tangent", 6);
        program->addBindAttribLocation("osg_Binormal", 7);

        ss->setAttributeAndModes(program.get());
        ss->setTextureAttributeAndModes(0, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(1, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(2, osgVerse::createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f)));
        ss->setTextureAttributeAndModes(3, osgVerse::createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        ss->setTextureAttributeAndModes(4, createTexture2D(osgDB::readImageFile("../PeiLi/barcelona.hdr")));
        ss->addUniform(new osg::Uniform("DiffuseMap", (int)0));
        ss->addUniform(new osg::Uniform("LightMap", (int)1));
        ss->addUniform(new osg::Uniform("NormalMap", (int)2));
        ss->addUniform(new osg::Uniform("SpecularMap", (int)3));
        ss->addUniform(new osg::Uniform("ReflectionMap", (int)4));
        ss->setMode(GL_BLEND, osg::StateAttribute::ON);
    }

    osg::Vec4 lightDir(1.0f, 1.0f, -1.0f, 0.0f), lightDir2(-1.0f, 1.0f, -0.5f, 0.0f);
    osg::Vec4 lightColor(4.0f, 4.0f, 3.8f, 1.0f), lightColor2(1.0f, 1.0f, 1.3f, 1.0f);
    lightDir.normalize(); lightDir2.normalize();

    ss->getOrCreateUniform("dLightColor", osg::Uniform::FLOAT_VEC4)->set(lightColor);
    ss->getOrCreateUniform("dLightColor2", osg::Uniform::FLOAT_VEC4)->set(lightColor2);

    // Scene viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    while (!viewer.done())
    {
        osg::Matrixf viewMatrix = viewer.getCamera()->getViewMatrix();
        ss->getOrCreateUniform("dLightDirection", osg::Uniform::FLOAT_VEC4)->set(lightDir * viewMatrix);
        ss->getOrCreateUniform("dLightDirection2", osg::Uniform::FLOAT_VEC4)->set(lightDir2 * viewMatrix);

        ss->getOrCreateUniform("ProjectionToView", osg::Uniform::FLOAT_MAT4)->set(
            osg::Matrixf::inverse(viewer.getCamera()->getProjectionMatrix()));
        viewer.frame();
    }
    return 0;
}
