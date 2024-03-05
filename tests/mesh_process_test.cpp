#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <modeling/DynamicGeometry.h>
#include <modeling/MeshTopology.h>
#include <modeling/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

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
        osgDB::writeNodeFile(*topoGeode, "topoResult.osg");

        topoMT->addChild(topoGeode.get());
        topoMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 20.0f));
    }

    // Test DynamicGeometry
    osg::ref_ptr<osgVerse::DynamicPolyline> lines = new osgVerse::DynamicPolyline(true);
    lines->addPoint(osg::Vec3(-5.0f, -5.0f, 0.0f));
    lines->addPoint(osg::Vec3(5.0f, -5.0f, 0.0f));
    lines->addPoint(osg::Vec3(5.0f, 5.0f, 0.0f));
    lines->addPoint(osg::Vec3(-5.0f, 5.0f, 0.0f));

    osg::ref_ptr<osg::Geode> lineGeode = new osg::Geode;
    lineGeode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    lineGeode->addDrawable(lines.get());

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());
    root->addChild(topoMT.get());
    root->addChild(lineGeode.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
