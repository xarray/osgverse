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

osg::Texture2D* createDefaultTexture(const osg::Vec4& color)
{
    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    image->setInternalTextureFormat(GL_RGBA);

    osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
    *ptr = osg::Vec4ub(color[0] * 255, color[1] * 255, color[2] * 255, 255);

    osg::ref_ptr<osg::Texture2D> tex2D = new osg::Texture2D;
    tex2D->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::NEAREST);
    tex2D->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::NEAREST);
    tex2D->setImage(image.get());
    return tex2D.release();
}

osg::Texture2D* loadTexture(std::string fileName)
{
    osg::Texture2D* tex = new osg::Texture2D(osgDB::readImageFile(fileName));
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    return tex;
}

void createPbrMaterial(osg::StateSet* ss, osg::Shader* vs, osg::Shader* fs,
                       osg::Texture2D* albedo, osg::Texture2D* metallic, osg::Texture2D* normal,
                       osg::Texture2D* roughness, osg::Texture2D* ao)
{
    osg::ref_ptr<osg::Program> prog = new osg::Program;
    prog->addShader(vs); prog->addShader(fs);
    ss->setAttributeAndModes(prog.get());
    ss->setTextureAttributeAndModes(0, albedo);
    ss->setTextureAttributeAndModes(1, metallic);
    ss->setTextureAttributeAndModes(2, normal);
    ss->setTextureAttributeAndModes(3, roughness);
    ss->setTextureAttributeAndModes(4, ao);
    ss->addUniform(new osg::Uniform("albedoMap", (int)0));
    ss->addUniform(new osg::Uniform("metallicMap", (int)1));
    ss->addUniform(new osg::Uniform("normalMap", (int)2));
    ss->addUniform(new osg::Uniform("roughnessMap", (int)3));
    ss->addUniform(new osg::Uniform("aoMap", (int)4));
}

int main(int argc, char** argv)
{
    osg::Geometry* geom = osg::createTexturedQuadGeometry(
        osg::Vec3(), osg::Vec3(1.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 1.0f, 0.0f), 0.0f, 0.0f, 1.0f, 1.0f);
    osg::ref_ptr<osg::Geode> quad = new osg::Geode;
    quad->addDrawable(geom);

    osg::ref_ptr<osg::MatrixTransform> model1 = new osg::MatrixTransform;
    model1->setMatrix(osg::Matrix::translate(-0.6f, 0.0f, 0.0f));
    model1->addChild(osgDB::readNodeFile("Gas_Cylinder.osg"));
    createPbrMaterial(model1->getOrCreateStateSet(),
        osgDB::readShaderFile(osg::Shader::VERTEX, "pbr/pbr.vert"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, "pbr/pbr.frag"),
        loadTexture("Albedo.png"), loadTexture("Metallic.png"),
        loadTexture("Normal.png"), loadTexture("Roughness.png"),
        loadTexture("Ao.png"));

    osg::ref_ptr<osg::MatrixTransform> model2 = new osg::MatrixTransform;
    model2->setMatrix(osg::Matrix::translate(0.6f, 0.0f, 0.0f));
    model2->addChild(quad.get());
    createPbrMaterial(model2->getOrCreateStateSet(),
        osgDB::readShaderFile(osg::Shader::VERTEX, "pbr/pbr.vert"),
        osgDB::readShaderFile(osg::Shader::FRAGMENT, "pbr/pbr.frag"),
        loadTexture("pbr/WoodReinforced1_AL.tif"), createDefaultTexture(osg::Vec4(0.0f, 0.0f, 0.0f, 1.0)),
        loadTexture("pbr/WoodReinforced1_N.tif"), loadTexture("pbr/WoodReinforced1_H.tif"),
        loadTexture("pbr/WoodReinforced1_AO.tif"));

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(model1.get());
    root->addChild(model2.get());

    osgViewer::Viewer viewer;
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
