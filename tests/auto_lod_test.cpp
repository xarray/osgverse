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

#include <readerwriter/Utilities.h>
#include <modeling/Utilities.h>
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

class CrossTreeVisitor : public osg::NodeVisitor
{
public:
    CrossTreeVisitor::CrossTreeVisitor()
        : osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN) {}

    virtual void apply(osg::Geode& node)
    {
        if (_toHandleNodes.find(&node) == _toHandleNodes.end())
            _toHandleNodes.insert(&node);
        traverse(node);
    }

    void handleCrossTrees(int w, int h, float lodValue)
    {
        osg::ref_ptr<osg::Group> root = new osg::Group;
        osgViewer::Viewer rttViewer;
        rttViewer.setSceneData(root.get());
        rttViewer.setUpViewInWindow(0, 0, 10, 10);

        for (std::set<osg::Geode*>::iterator itr = _toHandleNodes.begin();
             itr != _toHandleNodes.end(); ++itr)
        {
            osg::ref_ptr<osg::Geode> geode = *itr;
            osgVerse::MeshCollector mc; geode->accept(mc);
            std::cout << osgVerse::getNodePathID(*geode) << ": " << geode->referenceCount() << "\n";

            osg::BoundingBoxd bb = mc.getBoundingBox(); osg::Vec3 halfExtent = (bb._max - bb._min) * 0.5f;
            osg::Vec3 axis0 = osg::X_AXIS * halfExtent.x(), axis1 = osg::Y_AXIS * halfExtent.y();
            osg::Vec3 axis2 = osg::Z_AXIS * halfExtent.z();

            osg::ref_ptr<osg::Image> image0 = new osg::Image;
            osg::ref_ptr<osg::Image> image1 = new osg::Image;
            image0->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
            image1->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);

            osg::ref_ptr<osg::Camera> camera0 =
                osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER, image0.get(), NULL);
            osg::ref_ptr<osg::Camera> camera1 =
                osgVerse::createRTTCamera(osg::Camera::COLOR_BUFFER, image1.get(), NULL);
            root->removeChildren(0, root->getNumChildren());
            root->addChild(camera0.get()); root->addChild(camera1.get());

            // Align RTT cameras to XOZ and YOZ planes
            // A new geode with deep-copied resource is used with rttViewer separately
            osg::ref_ptr<osg::Geode> geodeNew = new osg::Geode(*geode, osg::CopyOp::DEEP_COPY_ALL);
            geodeNew->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

            osgVerse::alignCameraToBox(camera0.get(), bb, w, h, osg::TextureCubeMap::POSITIVE_X);
            osgVerse::alignCameraToBox(camera1.get(), bb, w, h, osg::TextureCubeMap::POSITIVE_Y);
            camera0->addChild(geodeNew.get()); camera1->addChild(geodeNew.get());
            for (unsigned int i = 0; i < 2; ++i) rttViewer.frame();

            // Create result cross-quad tree geode (with 4 parts to avoid wrong transparent sorting)
            osg::ref_ptr<osg::Texture> tex0 = osgVerse::createTexture2D(image0.get());
            osg::ref_ptr<osg::Texture> tex1 = osgVerse::createTexture2D(image1.get());
            osg::ref_ptr<osg::Geode> rough = new osg::Geode;
            rough->addDrawable(osg::createTexturedQuadGeometry(
                bb.center() - axis0 - axis2, axis0, axis2 * 2.0f, 0.5f, 1.0f));
            rough->addDrawable(osg::createTexturedQuadGeometry(
                bb.center() - axis2, axis0, axis2 * 2.0f, 0.5f, 0.0f, 1.0f, 1.0f));
            rough->addDrawable(osg::createTexturedQuadGeometry(
                bb.center() - axis1 - axis2, axis1, axis2 * 2.0f, 0.5f, 1.0f));
            rough->addDrawable(osg::createTexturedQuadGeometry(
                bb.center() - axis2, axis1, axis2 * 2.0f, 0.5f, 0.0f, 1.0f, 1.0f));
            for (unsigned int i = 0; i < rough->getNumDrawables(); ++i)
            {
                osg::StateSet* ss = rough->getDrawable(i)->getOrCreateStateSet();
                ss->setTextureAttributeAndModes(0, (i < 2) ? tex0.get() : tex1.get());
                ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
                ss->setMode(GL_BLEND, osg::StateAttribute::ON);
            }
            rough->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

            // Create a LOD node and replace current geode
            osg::ref_ptr<osg::LOD> lod = new osg::LOD;
            lod->setRangeMode(osg::LOD::DISTANCE_FROM_EYE_POINT);
            while (geode->getNumParents() > 0)
            {
                osg::Group* parent = geode->getParent(0);
                parent->replaceChild(geode, lod.get());
            }
            lod->addChild(rough.get(), lodValue, FLT_MAX);
            lod->addChild(geode.get(), 0.0f, lodValue);  // addChild() later to avoid being parent
        }
        rttViewer.setDone(true);
        _toHandleNodes.clear();
    }

protected:
    std::set<osg::Geode*> _toHandleNodes;
};

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osgVerse::updateOsgBinaryWrappers();

    std::string treeFileName; arguments.read("--tree", treeFileName);
    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(treeFileName);
    if (!scene) { OSG_WARN << "Failed to load tree file" << treeFileName; return 1; }

    CrossTreeVisitor ctv;
    scene->accept(ctv);
    ctv.handleCrossTrees(1024, 1024, 80.0f);
    osgDB::writeNodeFile(*scene, treeFileName + "_crossed.ive");

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(scene.get());
    return viewer.run();
}
