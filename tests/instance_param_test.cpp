#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/VertexAttribDivisor>
#include <osg/Geometry>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Utilities.h>
#include <pipeline/Pipeline.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#define RAND_VALUE(m, n) ((n - m) * (float)rand() / (float)RAND_MAX + m)
#define RES 2048
#define RESV "2048"

char* vertCode0 = {
    "#extension GL_EXT_draw_instanced : enable\n"
    "VERSE_VS_IN vec4 osg_UserPosition, osg_UserColor;\n"
    "VERSE_VS_OUT vec4 color;\n"
    "void main() {\n"
    "    float r = float(gl_InstanceID) / " RESV ";\n"
    "    float c = floor(r) / " RESV "; r = fract(r);\n"
    "    vec4 pos = osg_UserPosition;\n"
    "    color = osg_UserColor;\n"
    "    gl_Position = VERSE_MATRIX_MVP * (osg_Vertex + vec4(pos.xyz, 0.0));\n"
    "}\n"
};

char* vertCode1 = {
    "#extension GL_EXT_draw_instanced : enable\n"
    "uniform sampler2D PosTexture, ColorTexture;\n"
    "VERSE_VS_OUT vec4 color;\n"
    "void main() {\n"
    "    float r = float(gl_InstanceID) / " RESV ";\n"
    "    float c = floor(r) / " RESV "; r = fract(r);\n"
    "    vec4 pos = VERSE_TEX2D(PosTexture, vec2(r, c));\n"
    "    color = VERSE_TEX2D(ColorTexture, vec2(r, c));\n"
    "    gl_Position = VERSE_MATRIX_MVP * (osg_Vertex + vec4(pos.xyz, 0.0));\n"
    "}\n"
};

char* fragCode = {
    "VERSE_FS_IN vec4 color;\n"
    "VERSE_FS_OUT vec4 fragData;\n"
    "void main() {\n"
    "    fragData = color;\n"
    "    VERSE_FS_FINAL(fragData);\n"
    "}\n"
};

osg::Geometry* createInstancedGeometry()
{
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    va->push_back(osg::Vec3(-1.0f, 0.0f, -1.0f));
    va->push_back(osg::Vec3(1.0f, 0.0f, -1.0f));
    va->push_back(osg::Vec3(1.0f, 1.0f, 1.0f));
    va->push_back(osg::Vec3(-1.0f, 1.0f, 1.0f));

    osg::ref_ptr<osg::DrawElementsUShort> de = new osg::DrawElementsUShort(GL_TRIANGLES);
    de->push_back(0); de->push_back(1); de->push_back(2);
    de->push_back(0); de->push_back(2); de->push_back(3);
    de->setNumInstances(RES * RES);

    // Create the geometry
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
    geom->addPrimitiveSet(de.get());
    return geom.release();
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments(&argc, argv);
    bool useVertexAttribDivisor = (!arguments.read("--texture"));

    osg::Geometry* geom = createInstancedGeometry();
    osg::ref_ptr<osg::Geode> root = new osg::Geode;
    root->addChild(geom);

    // Update instanced quad parameters
    std::vector<osg::Vec3> positions(RES * RES);
    std::vector<osg::Vec4ub> colors(RES * RES);
    osg::BoundingBox bb;
    for (unsigned int i = 0; i < colors.size(); ++i)
    {
        unsigned int x = (i % RES), y = (i / RES);
        positions[i] = osg::Vec3(x * 2.0f, RAND_VALUE(-1.0f, 1.0f), y * 2.0f);
        colors[i] = osg::Vec4ub(x % 255, y % 255, x / 255 + y / 255, 1.0f);
        bb.expandBy(positions[i]);
    }
    geom->setInitialBound(bb);

    osg::ref_ptr<osg::Vec3Array> posArray = new osg::Vec3Array(positions.size());
    osg::ref_ptr<osg::Vec4ubArray> colorArray = new osg::Vec4ubArray(colors.size());

    osg::ref_ptr<osg::Image> posImage = new osg::Image;
    osg::ref_ptr<osg::Image> colorImage = new osg::Image;

    osg::StateSet* ss = geom->getOrCreateStateSet();
    if (useVertexAttribDivisor)
    {
        // A little better than textured: 45-50fps on GTX 3050Ti
        geom->setVertexAttribArray(1, posArray.get());
        geom->setVertexAttribArray(2, colorArray.get());
        geom->setVertexAttribBinding(1, osg::Geometry::BIND_PER_VERTEX);
        geom->setVertexAttribBinding(2, osg::Geometry::BIND_PER_VERTEX);
        geom->setVertexAttribNormalize(1, false);
        geom->setVertexAttribNormalize(2, true);

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addBindAttribLocation("osg_UserPosition", 1);
        prog->addBindAttribLocation("osg_UserColor", 2);
        prog->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode0));
        prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));
        osgVerse::Pipeline::createShaderDefinitions(prog->getShader(0), 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(prog->getShader(1), 100, 130);  // FIXME
        ss->setAttribute(prog.get());
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(1, 1));
        ss->setAttributeAndModes(new osg::VertexAttribDivisor(2, 1));
    }
    else
    {
        // Large non-standard textures will cause low effectivity: ~30fps on GTX 3050Ti
        posImage->allocateImage(RES, RES, 1, GL_RGB, GL_FLOAT);
        posImage->setInternalTextureFormat(GL_RGB32F_ARB);
        colorImage->allocateImage(RES, RES, 1, GL_RGBA, GL_UNSIGNED_BYTE);
        colorImage->setInternalTextureFormat(GL_RGBA8);

        osg::Texture2D* tex0 = osgVerse::createTexture2D(posImage.get(), osg::Texture::REPEAT);
        tex0->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        tex0->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        ss->setTextureAttributeAndModes(0, tex0);
        ss->addUniform(new osg::Uniform("PosTexture", (int)0));

        osg::Texture2D* tex1 = osgVerse::createTexture2D(colorImage.get(), osg::Texture::REPEAT);
        tex1->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        tex1->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        ss->setTextureAttributeAndModes(1, tex1);
        ss->addUniform(new osg::Uniform("ColorTexture", (int)1));

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(new osg::Shader(osg::Shader::VERTEX, vertCode1));
        prog->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));
        osgVerse::Pipeline::createShaderDefinitions(prog->getShader(0), 100, 130);
        osgVerse::Pipeline::createShaderDefinitions(prog->getShader(1), 100, 130);  // FIXME
        ss->setAttribute(prog.get());
    }

    // Start the viewer
    osg::ref_ptr<osgViewer::Viewer> viewer = new osgViewer::Viewer;
    viewer->addEventHandler(new osgViewer::StatsHandler);
    viewer->addEventHandler(new osgViewer::WindowSizeHandler);
    viewer->setCameraManipulator(new osgGA::TrackballManipulator);
    viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer->setUpViewInWindow(50, 50, 800, 600);
    viewer->setSceneData(root.get());
    while (!viewer->done())
    {
        if (useVertexAttribDivisor)
        {
            memcpy(&(*posArray)[0], &positions[0], positions.size() * sizeof(osg::Vec3));
            memcpy(&(*colorArray)[0], &colors[0], colors.size() * sizeof(osg::Vec4ub));
            posArray->dirty(); colorArray->dirty();
        }
        else
        {
            osg::Vec3* ptr0 = (osg::Vec3*)posImage->data();
            osg::Vec4ub* ptr1 = (osg::Vec4ub*)colorImage->data();
            memcpy(ptr0, &positions[0], positions.size() * sizeof(osg::Vec3));
            memcpy(ptr1, &colors[0], colors.size() * sizeof(osg::Vec4ub));
            posImage->dirty(); colorImage->dirty();
        }
        viewer->frame();
    }
    return 0;
}
