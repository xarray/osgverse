#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <modeling/MeshTopology.h>
#include <modeling/Utilities.h>
#include <iostream>
#include <sstream>

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

    // Remove all small frac
    std::vector<std::vector<uint32_t>> entities = topology->getEntityFaces();
    for (size_t i = 0; i < entities.size(); ++i)
    {
        std::vector<uint32_t>& faces = entities[i];
        if (faces.size() > 40) continue;

        for (size_t j = 0; j < faces.size(); ++j)
            topology->deleteFace(faces[j]);
    }
    topology->prune();

    osg::ref_ptr<osg::MatrixTransform> topoMT = new osg::MatrixTransform;
    {
        osg::ref_ptr<osg::Geode> topoGeode = new osg::Geode;
        topoGeode->addDrawable(topology->output());
        topoGeode->setStateSet(mtv.getMergedStateSet());
        topoMT->addChild(topoGeode.get());
        topoMT->setMatrix(osg::Matrix::translate(0.0f, 0.0f, 50.0f));
    }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());
    root->addChild(topoMT.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
