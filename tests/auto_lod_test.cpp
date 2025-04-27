#include PREPENDED_HEADER
#include <osg/io_utils>
#include <osg/ValueObject>
#include <osg/Texture2D>
#include <osg/Shape>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/ConvertUTF>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgText/Font>
#include <osgText/Text>

#include <VerseCommon.h>
#include <modeling/Octree.h>
#include <modeling/GeometryMerger.h>
#include <pipeline/IntersectionManager.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

struct DefaultGpuBaker : public osgVerse::GeometryMerger::GpuBaker
{
    virtual osg::Image* bakeTextureImage(osg::Node* node)
    {
        osg::ref_ptr<osg::Image> image = osgVerse::createSnapshot(node, 1024, 1024);
        return image.release();
    }

    virtual osg::Geometry* bakeGeometry(osg::Node* node)
    {
        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setUseDisplayList(false);
        geom->setUseVertexBufferObjects(true);

        osg::ref_ptr<osg::HeightField> hf = osgVerse::createHeightField(node, 128, 128);
        osgVerse::ShapeGeometryVisitor bsgv(geom.get(), NULL); hf->accept(bsgv);
        return geom.release();
    }
};

class SelectSceneHandler : public osgGA::GUIEventHandler
{
public:
    SelectSceneHandler(osg::Geode* tNode) : _textGeode(tNode)
    { _condition.nodesToIgnore.insert(_textGeode.get()); }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        osgViewer::View* view = static_cast<osgViewer::View*>(&aa);
        if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
            (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL))
        {
            osgVerse::IntersectionResult result = osgVerse::findNearestIntersection(
                view->getCamera(), ea.getXnormalized(), ea.getYnormalized(), &_condition);
            if (!result.drawable) return false;

            osgText::Text* text = NULL;
            if (_textGeode->getNumDrawables() == 0)
            {
                text = new osgText::Text;
                text->setPosition(osg::Vec3(10.0f, 40.0f, 0.0f));
                text->setCharacterSize(20.0f, 1.0f);
                text->setFont(MISC_DIR + "LXGWFasmartGothic.ttf");
                _textGeode->addDrawable(text);
            }
            else
                text = static_cast<osgText::Text*>(_textGeode->getDrawable(0));

            std::wstring t = osgDB::convertUTF8toUTF16("HIT: " + result.drawable->getName());
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
            if (result.drawable->getUserDataContainer())
                t += getUserString(result.drawable->getUserDataContainer());
            else if (!result.intersectIndirectData.empty())
            {
                osgVerse::IntersectionResult::IndirectData& id = result.intersectIndirectData[0];
                t += getUserString(id.first->getUserData(id.second));
            }
#endif
            text->setText(t.c_str());
        }
        return false;
    }

#if OSG_VERSION_GREATER_THAN(3, 4, 1)
    std::wstring getUserString(osg::UserDataContainer* udc)
    {
        std::wstring text;
        if (udc != NULL)
        {
            osg::StringValueObject* svo =
                dynamic_cast<osg::StringValueObject*>(udc->getUserObject("Index"));
            text += osgDB::convertUTF8toUTF16("; DATA: " + udc->getName());
            if (svo) text += osgDB::convertUTF8toUTF16(", " + svo->getValue());
        }
        return text;
    }
#endif

protected:
    osg::observer_ptr<osg::Geode> _textGeode;
    osgVerse::IntersectionCondition _condition;
};

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
    osg::ref_ptr<osg::Group> root = new osg::Group;

#if true
    osg::Geometry* g0 = osg::createTexturedQuadGeometry(osg::Vec3(0.0f, 0.0f, 0.0f), osg::X_AXIS, osg::Y_AXIS);
    osg::Geometry* g1 = osg::createTexturedQuadGeometry(osg::Vec3(1.0f, 0.0f, 0.0f), osg::X_AXIS, osg::Y_AXIS);
    osg::Geometry* g2 = osg::createTexturedQuadGeometry(osg::Vec3(1.0f, 1.0f, 0.0f), osg::X_AXIS, osg::Y_AXIS);
    osg::Geometry* g3 = osg::createTexturedQuadGeometry(osg::Vec3(0.0f, 1.0f, 0.0f), osg::X_AXIS, osg::Y_AXIS);

    g0->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, osgVerse::createTexture2D(osgDB::readImageFile("Images/blueFlowers.png")));
    g1->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, osgVerse::createTexture2D(osgDB::readImageFile("Images/forestRoof.png")));
    g2->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, osgVerse::createTexture2D(osgDB::readImageFile("Images/osgshaders1.png")));
    g3->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, osgVerse::createTexture2D(osgDB::readImageFile("Images/skymap.jpg")));

    std::vector<std::pair<osg::Geometry*, osg::Matrix>> testList;
    testList.push_back(std::pair<osg::Geometry*, osg::Matrix>(g0, osg::Matrix()));
    testList.push_back(std::pair<osg::Geometry*, osg::Matrix>(g1, osg::Matrix()));
    testList.push_back(std::pair<osg::Geometry*, osg::Matrix>(g2, osg::Matrix()));
    testList.push_back(std::pair<osg::Geometry*, osg::Matrix>(g3, osg::Matrix()));

    osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::GPU_BAKING);
    merger.setGpkBaker(new DefaultGpuBaker);

    osg::ref_ptr<osg::Geometry> merged = merger.process(testList, 0);
    osgDB::writeNodeFile(*merged, "test_merging.osgb"); return 0;
#endif

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

                    osg::Geometry* geom = osgVerse::createPrism(center, 1.0f, 1.0f, height);
                    geom->setName("Prism_" + std::to_string(group->getNumChildren()) +
                                  "_" + std::to_string(mt->getNumChildren()));
                    geom->setUserValue("Index",
                        std::to_string(x) + "_" + std::to_string(y) + "_" + std::to_string(z));

                    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
                    geode->addDrawable(geom); mt->addChild(geode.get());
                }
            }
    }

    // Find all geometries and merge them
    osgVerse::FixedFunctionOptimizer ffo; scene->accept(ffo);
    FindGeometryVisitor fgv(true); scene->accept(fgv);

    osg::ref_ptr<osg::Geometry> newGeom;
    if (arguments.read("--original")) {}  // do nothing
    else if (arguments.read("--combine"))
    {
        osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::COMBINED_GEOMETRY);
        merger.setForceColorArray(true);
        newGeom = merger.process(fgv.geomList, 0);
    }
    else if (arguments.read("--indirect"))
    {
        osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::INDIRECT_COMMANDS);
        merger.setForceColorArray(true);
        newGeom = merger.process(fgv.geomList, 0);
    }
    else  // octree mode
    {
        osg::ref_ptr<osg::Geode> octRoot = new osg::Geode;
        octRoot->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        octRoot->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        octRoot->getOrCreateStateSet()->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        root->addChild(octRoot.get());

        osgVerse::GeometryMerger merger(osgVerse::GeometryMerger::INDIRECT_COMMANDS);
        merger.setForceColorArray(true);
        scene = merger.processAsOctree(fgv.geomList, 0, 0, 4096, octRoot.get(), 100,
                                       osg::maximum(50.0f, scene->getBound().radius() * 0.02f));
    }

    if (newGeom.valid())
    {
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        geode->addDrawable(newGeom.get()); scene = geode;
    }

    // Start the viewer
    osg::ref_ptr<osg::Geode> textGeode = new osg::Geode;
    osg::ref_ptr<osg::Camera> postCamera = osgVerse::createHUDCamera(NULL, 1920, 1080);
    postCamera->addChild(textGeode.get());
    root->addChild(postCamera.get());
    root->addChild(scene.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new SelectSceneHandler(textGeode.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
