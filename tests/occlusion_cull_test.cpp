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

#include <VerseCommon.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Rasterizer.h>
#include <pipeline/Pipeline.h>

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

#if true
    osg::ref_ptr<osg::Node> terrain = osgDB::readNodeFile("lz.osg");
    osg::ref_ptr<osg::Node> cessna = osgDB::readNodeFile("cessna.osg");
    if (!terrain || !cessna) return 1;

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    terrain->setName("TERRAIN"); root->addChild(terrain.get());
    cessna->setName("CESSNA"); root->addChild(cessna.get());
    
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());

    osg::ref_ptr<osgVerse::UserOccluder> occ1 = new osgVerse::UserOccluder(*terrain);
    osg::ref_ptr<osgVerse::UserOccluder> occ2 = new osgVerse::UserOccluder(*cessna);

    osg::ref_ptr<osgVerse::UserRasterizer> rasterizer = new osgVerse::UserRasterizer(1280, 720);
    rasterizer->addOccluder(occ1.get()); rasterizer->addOccluder(occ2.get());

    std::vector<float> depthData; std::vector<unsigned short> hizData;
    while (!viewer.done())
    {
        osg::Vec3 cameraPos = osg::Vec3() * viewer.getCamera()->getInverseViewMatrix();
        rasterizer->setModelViewProjection(viewer.getCamera()->getViewMatrix(),
                                           viewer.getCamera()->getProjectionMatrix());
        rasterizer->render(cameraPos, &depthData, &hizData);

        float vis = rasterizer->queryVisibility(occ2.get());
        std::cout << "Cessna Visibility: " << vis << "\n";
        viewer.frame();
    }
#else
    // test with original https://github.com/rawrunprotected/rasterizer
    std::vector<__m128> vertices; std::vector<uint32_t> indices;
    {
        std::stringstream fileName; fileName << "../Castle/IndexBuffer.bin";
        std::ifstream inFile(fileName.str(), std::ifstream::binary);

        inFile.seekg(0, std::ifstream::end);
        auto size = inFile.tellg(); inFile.seekg(0);
        auto numIndices = size / sizeof indices[0];
        indices.resize(numIndices);
        inFile.read(reinterpret_cast<char*>(&indices[0]), numIndices * sizeof indices[0]);
    }

    {
        std::stringstream fileName; fileName << "../Castle/VertexBuffer.bin";
        std::ifstream inFile(fileName.str(), std::ifstream::binary);

        inFile.seekg(0, std::ifstream::end);
        auto size = inFile.tellg(); inFile.seekg(0);
        auto numVertices = size / sizeof vertices[0];
        vertices.resize(numVertices);
        inFile.read(reinterpret_cast<char*>(&vertices[0]), numVertices * sizeof vertices[0]);
    }

    osg::ref_ptr<osgVerse::UserOccluder> occ = new osgVerse::UserOccluder("Castle", vertices, indices);
    osg::ref_ptr<osgVerse::UserRasterizer> rasterizer = new osgVerse::UserRasterizer(1280, 720);
    rasterizer->addOccluder(occ.get());

    std::vector<float> depthData;
    osg::Vec3 cameraPos(27.0f, 2.0f, 47.0f);
    osg::Matrix proj = osg::Matrix::perspective(osg::RadiansToDegrees(0.628f), 1280.0f / 720.0f, 1.0f, 5000.0f);
    osg::Matrix view = osg::Matrix::lookAt(
        cameraPos, cameraPos + osg::Vec3(0.142582759f, 0.0611068942f, -0.987894833f), osg::Vec3(0.0f, 1.0f, 0.0f));
    rasterizer->setModelViewProjection(view, proj);
    rasterizer->render(cameraPos, &depthData, NULL);
#endif

    osg::ref_ptr<osg::Image> image = new osg::Image;
    image->allocateImage(1280, 720, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    {
        osg::Vec4ub* ptr = (osg::Vec4ub*)image->data();
        for (int y = 0; y < image->t(); ++y)
            for (int x = 0; x < image->s(); ++x)
            {
                unsigned char mid = 0, value = 0, alpha = 255;
                float depth = depthData[y * image->s() + x] * 100.0f;
                if (depth > 1.0f) { mid = (unsigned char)(floor(depth) * 2.55f); depth *= 0.01f; }
                else if (depth < 0.0f) { alpha = 0; depth = 0.0f; }

                value = (unsigned char)(255.0f * depth);
                *(ptr + y * image->s() + x) = osg::Vec4ub(value, mid, 0, alpha);
            }
    }
    osgDB::writeImageFile(*image, "test_occlusion.png");
    return 0;
}
