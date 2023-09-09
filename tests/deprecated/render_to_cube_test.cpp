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
#include <pipeline/SkyBox.h>
#include <pipeline/Pipeline.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::TextureCubeMap> tex = new osg::TextureCubeMap;
    tex->setTextureSize(1024, 1024);
    osgVerse::Pipeline::setTextureBuffer(tex, osgVerse::Pipeline::BufferType::RGBA_INT8);

    // Scene to render-to-texture
    osg::ref_ptr<osg::Group> scene = new osg::Group;
    scene->addChild(osgDB::readNodeFile("lz.osg"));

    // Skybox to display the result
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::SkyBox::createSkyCamera();
    {
        osg::ref_ptr<osgVerse::SkyBox> skybox = new osgVerse::SkyBox;
        skybox->setSkyShaders(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "skybox.vert.glsl"),
                              osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "skybox.frag.glsl"));
        skybox->setEnvironmentMap(tex.get(), false);
        postCamera->addChild(skybox.get());
    }

    // Scene root
    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(osgVerse::createRTTCube(osg::Camera::COLOR_BUFFER, tex, scene.get(), NULL));
    root->addChild(postCamera.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
