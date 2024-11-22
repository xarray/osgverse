#include <osg/io_utils>
#include <osg/Multisample>
#include <osg/Texture2D>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Octree.h>
#include <modeling/GeometryMerger.h>
#include <modeling/Utilities.h>
#include <readerwriter/Utilities.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class FindGeometryVisitor : public osg::NodeVisitor
{
public:
    FindGeometryVisitor(bool applyM)
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), appliedMatrix(applyM) {}
    inline void pushMatrix(osg::Matrix& matrix) { _matrixStack.push_back(matrix); }
    inline void popMatrix() { _matrixStack.pop_back(); }

    std::vector<osg::Matrix> _matrixStack;
    std::vector<std::pair<osg::Geometry*, osg::Matrix>> geomList;
    bool appliedMatrix;

    virtual void apply(osg::Transform& node)
    {
        osg::Matrix matrix;
        if (!_matrixStack.empty()) matrix = _matrixStack.back();
        node.computeLocalToWorldMatrix(matrix, this);
        pushMatrix(matrix); traverse(node); popMatrix();
    }

    virtual void apply(osg::Geode& node)
    {
        osg::Matrix matrix;
        if (appliedMatrix && _matrixStack.size() > 0) matrix = _matrixStack.back();
        for (size_t i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom) geomList.push_back(std::pair<osg::Geometry*, osg::Matrix>(geom, matrix));
        }
        traverse(node);
    }
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene)
    {
        osg::ref_ptr<osg::Group> group = new osg::Group; scene = group;
        for (int z = 0; z < 40; ++z)
            for (int y = 0; y < 40; ++y)
            {
                osg::ref_ptr<osg::MatrixTransform> mt = new osg::MatrixTransform;
                mt->setMatrix(osg::Matrix::translate(10.0f * y * (float)rand() / (float)RAND_MAX,
                                                     10.0f * z * (float)rand() / (float)RAND_MAX, 0.0f));
                group->addChild(mt.get());

                for (int x = 0; x < 40; ++x)
                {
                    osg::Vec3 center(x * (float)rand() / (float)RAND_MAX,
                                     y * (float)rand() / (float)RAND_MAX,
                                     z * (float)rand() / (float)RAND_MAX);
                    float height = (float)rand() / (float)RAND_MAX;

                    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
                    geode->addDrawable(osgVerse::createPrism(center, 1.0f, 1.0f, height));
                    mt->addChild(geode.get());
                }
            }
    }

    // Find all geometries and merge them
    FindGeometryVisitor fgv(true);
    scene->accept(fgv);

    osg::ref_ptr<osg::Geometry> newGeom;
    if (arguments.read("--original")) {}  // do nothing
    else if (arguments.read("--combine"))
    {
        osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::COMBINED_GEOMETRY);
        newGeom = merger.process(fgv.geomList, 0);
    }
    else if (arguments.read("--indirect"))
    {
        osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::INDIRECT_COMMANDS);
        newGeom = merger.process(fgv.geomList, 0);
    }
    else  // octree mode
    {

    }

    if (newGeom.valid())
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(newGeom.get()); scene = geode;
    }

    // Start the viewer
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(scene.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
