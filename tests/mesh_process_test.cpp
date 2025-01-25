#include <osg/io_utils>
#include <osg/PolygonMode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/SmoothingVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <modeling/DynamicGeometry.h>
#include <modeling/MeshTopology.h>
#include <modeling/GeometryMapper.h>
#include <modeling/Utilities.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#define TEST_MAPPING_TO_VHACD 1
#define TEST_MODELING_FUNCTIONS 1

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    osgVerse::MeshTopologyVisitor mtv;
    mtv.setWeldingVertices(true);
    scene->accept(mtv);

    osg::ref_ptr<osgVerse::MeshTopology> topology = mtv.generate();
    //topology->simplify(0.8f);

    std::vector<std::vector<uint32_t>> entities = topology->getEntityFaces();
#if 0
    for (size_t i = 0; i < entities.size(); ++i)
    {
        std::vector<uint32_t>& faces = entities[i];
        if (faces.size() > 40) continue;

        for (size_t j = 0; j < faces.size(); ++j)
            topology->deleteFace(faces[j]);
    }
    topology->prune();
#endif

#if !TEST_MAPPING_TO_VHACD
    osg::ref_ptr<osg::MatrixTransform> topoMT = new osg::MatrixTransform;
    {
        static osg::Vec4 colors[] = {
            osg::Vec4(0.8f, 0.8f, 0.8f, 1.0f), osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f),
            osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f), osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f),
            osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f), osg::Vec4(0.0f, 1.0f, 1.0f, 1.0f),
            osg::Vec4(1.0f, 0.0f, 1.0f, 1.0f), osg::Vec4(0.2f, 0.2f, 0.2f, 1.0f)
        };

        char* fragCode = {
            "uniform vec4 color;\n"
            "void main() {\n"
            "    gl_FragColor = gl_Color * color;\n"
            "}\n"
        };
        osg::ref_ptr<osg::Program> program = new osg::Program;
        program->addShader(new osg::Shader(osg::Shader::FRAGMENT, fragCode));

        osg::ref_ptr<osg::Geode> topoGeode = topology->outputByEntity();
        for (size_t i = 0; i < topoGeode->getNumDrawables(); ++i)
        {
            std::cout << "Entity-" << i << ": Faces = " << entities[i].size() << std::endl;
            topoGeode->getDrawable(i)->getOrCreateStateSet()->addUniform(
                new osg::Uniform("color", colors[i % 8]));
        }
        topoGeode->setStateSet(mtv.getMergedStateSet());
        topoGeode->getOrCreateStateSet()->setAttributeAndModes(program.get());
        //osgDB::writeNodeFile(*topoGeode, "topoResult.osg");

        topoMT->addChild(topoGeode.get());
        topoMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 20.0f));
    }

    // Test DynamicGeometry
    osg::ref_ptr<osgVerse::DynamicPolyline> lines = new osgVerse::DynamicPolyline(true);
    lines->addPoint(osg::Vec3(-5.0f, -5.0f, 0.0f));
    lines->addPoint(osg::Vec3(5.0f, -5.0f, 0.0f));
    lines->addPoint(osg::Vec3(5.0f, 5.0f, 0.0f));
    lines->addPoint(osg::Vec3(-5.0f, 5.0f, 0.0f));
#endif

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    //root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

#if TEST_MAPPING_TO_VHACD
    osgVerse::BoundingVolumeVisitor bvv; scene->accept(bvv);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(bvv.computeVHACD());
    root->addChild(geode.get());
    osgUtil::SmoothingVisitor smv; geode->accept(smv);

    osgVerse::GeometryMapper mapper;
    std::cout << "Similarity: " << mapper.computeSimilarity(scene.get(), geode.get()) << std::endl;
    mapper.mapAttributes(scene.get(), geode.get());

    osg::ref_ptr<osg::MatrixTransform> sceneMT = new osg::MatrixTransform;
    sceneMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 20.0f));
    sceneMT->addChild(scene.get());
    root->addChild(sceneMT.get());
#else
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet()->setAttributeAndModes(
        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));
    geode->addDrawable(lines.get());
    root->addChild(geode.get());

    // Test VHACD
    osgVerse::BoundingVolumeVisitor bvv; scene->accept(bvv);
    geode->addDrawable(bvv.computeVHACD());
    root->addChild(topoMT.get());
#endif

#if TEST_MODELING_FUNCTIONS
    {
        std::vector<osg::Vec3d> pathL;
        pathL.push_back(osg::Vec3(1.0f, 0.0f, 5.0f));
        pathL.push_back(osg::Vec3(4.0f, 0.0f, 4.0f));
        pathL.push_back(osg::Vec3(6.0f, 0.0f, 2.0f));
        pathL.push_back(osg::Vec3(7.0f, 0.0f, 0.0f));
        osg::ref_ptr<osg::Geometry> geomL =
            osgVerse::createLatheGeometry(pathL, osg::Z_AXIS, 16, true);

        std::vector<osg::Vec3d> pathE;
        std::vector<std::vector<osg::Vec3d>> pathEinner;
        pathE.push_back(osg::Vec3(10.0f, 0.0f, 0.0f));
        pathE.push_back(osg::Vec3(15.0f, 0.0f, 0.0f));
        pathE.push_back(osg::Vec3(15.0f, 5.0f, 0.0f));
        pathE.push_back(osg::Vec3(10.0f, 5.0f, 0.0f));
        {
            std::vector<osg::Vec3d> pathI0;
            pathI0.push_back(osg::Vec3(11.0f, 1.0f, 0.0f));
            pathI0.push_back(osg::Vec3(14.0f, 1.0f, 0.0f));
            pathI0.push_back(osg::Vec3(14.0f, 4.0f, 0.0f));
            pathI0.push_back(osg::Vec3(12.0f, 4.0f, 0.0f));
            pathEinner.push_back(pathI0);
        }
        osg::ref_ptr<osg::Geometry> geomE =
            osgVerse::createExtrusionGeometry(pathE, pathEinner, osg::Z_AXIS * 5.0f, true);

        std::vector<osg::Vec3d> pathLo;
        std::vector<std::vector<osg::Vec3d>> sectionsLo;
        pathLo.push_back(osg::Vec3(20.0f, 0.0f, 0.0f));
        pathLo.push_back(osg::Vec3(25.0f, 10.0f, 0.0f));
        pathLo.push_back(osg::Vec3(25.0f, 10.0f, 10.0f));
        pathLo.push_back(osg::Vec3(30.0f, 5.0f, 5.0f));
        {
            std::vector<osg::Vec3d> sec0;
            sec0.push_back(osg::Vec3(-1.0f, -1.0f, 0.0f));
            sec0.push_back(osg::Vec3(1.0f, -1.0f, 0.0f));
            sec0.push_back(osg::Vec3(1.0f, 1.0f, 0.0f));
            sec0.push_back(osg::Vec3(-1.0f, 1.0f, 0.0f));
            sectionsLo.push_back(sec0);
        }
        osg::ref_ptr<osg::Geometry> geomLo =
            osgVerse::createLoftGeometry(pathLo, sectionsLo, true, true);

        osg::ref_ptr<osg::Geode> geode1 = new osg::Geode;
        geode1->getOrCreateStateSet()->setTextureAttributeAndModes(
            0, osgVerse::createTexture2D(osgDB::readImageFile(BASE_DIR + "/textures/uv.jpg")));
        geode1->addDrawable(geomL.get());
        geode1->addDrawable(geomE.get());
        geode1->addDrawable(geomLo.get());
        //osgDB::writeNodeFile(*geode1, "test_mesh.osg");

        osg::ref_ptr<osg::MatrixTransform> mt1 = new osg::MatrixTransform;
        mt1->setMatrix(osg::Matrix::translate(20.0f, 0.0f, 0.0f));
        mt1->addChild(geode1.get());
        root->addChild(mt1.get());
    }
#endif

    /*osg::Image* image = new osg::Image;
    image->allocateImage(1024, 512, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    osg::Camera* camera = osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER0, image, scene);
    osgVerse::alignCameraToBox(camera, bvv.getBoundingBox(), 1024, 512);
    root->addChild(camera);*/

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
