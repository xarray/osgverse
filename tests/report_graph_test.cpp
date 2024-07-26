#include <osg/io_utils>
#include <osg/TriangleIndexFunctor>
#include <osg/Texture2D>
#include <osg/PagedLOD>
#include <osg/ProxyNode>
#include <osg/MatrixTransform>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <pipeline/Utilities.h>
#include <readerwriter/Utilities.h>
#include <modeling/Utilities.h>
#include <modeling/Math.h>
#include <iostream>
#include <sstream>

#ifdef OSG_LIBRARY_STATIC
USE_OSG_PLUGINS()
USE_VERSE_PLUGINS()
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class Reporter : public osg::NodeVisitor
{
public:
    Reporter::Reporter()
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _indent(0) {}

    virtual void apply(osg::Node& node)
    {
        outputBasic(node); pushIndent();
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
        traverse(node); popIndent();
    }

    virtual void apply(osg::PagedLOD& node)
    {
        outputBasic(node); pushIndent();
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
        std::cout << std::string(_indent, ' ') << "<Strategy> "
                  << (node.getCenterMode() == osg::LOD::USE_BOUNDING_SPHERE_CENTER ? "Child / " : "User / ")
                  << (node.getRangeMode() == osg::LOD::DISTANCE_FROM_EYE_POINT ? "Distance" : "Pixels");
        if (node.getCenterMode() == osg::LOD::USE_BOUNDING_SPHERE_CENTER) std::cout << std::endl;
        else std::cout << ", Bound = " << node.getCenter() << ", R = " << node.getRadius() << std::endl;

        unsigned int maxChild = node.getNumFileNames() - 1; traverse(node);
        for (unsigned int i = node.getNumChildren(); i <= maxChild; ++i)
        {
            std::string fileName = node.getDatabasePath() + node.getFileName(i);
            std::cout << std::string(_indent, ' ') << "<File " << i << "/" << maxChild << ">: "
                      << fileName << ", Exist = " << !osgDB::findDataFile(fileName).empty() << "; Min = "
                      << node.getMinRange(i) << ", Max = " << node.getMaxRange(i) << std::endl;

            osg::ref_ptr<osg::Node> child = osgDB::readNodeFile(fileName);
            if (child.valid())
            {
                pushIndent(); outputBasic(*child);
                std::cout << std::string(_indent, ' ') << "<Bound> " << child->getBound().center()
                          << ", R = " << child->getBound().radius() << std::endl;;
                popIndent();
            }
        }
        popIndent();
    }

    virtual void apply(osg::ProxyNode& node)
    {
        outputBasic(node); pushIndent();
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());

        unsigned int maxChild = node.getNumFileNames() - 1; traverse(node);
        for (unsigned int i = node.getNumChildren(); i <= maxChild; ++i)
        {
            std::string fileName = node.getDatabasePath() + node.getFileName(i);
            std::cout << std::string(_indent, ' ') << "<File " << i << "/" << maxChild << ">: "
                      << fileName << ", Exist = " << !osgDB::findDataFile(fileName).empty() << std::endl;
        }
        popIndent();
    }

    virtual void apply(osg::Transform& node)
    {
        osg::Matrix matrix, matrixL;
        if (!_matrixStack.empty()) matrix = _matrixStack.back();
        outputBasic(node); pushIndent();
        node.computeLocalToWorldMatrix(matrix, this);
        node.computeLocalToWorldMatrix(matrixL, this);
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());

        if (matrixL != matrix)
        {
            osg::Vec3 pos, scale; osg::Quat rot, so; matrixL.decompose(pos, rot, scale, so);
            osg::Vec3d euler = osgVerse::computeHPRFromQuat(rot);
            std::cout << std::string(_indent, ' ') << "<Local>: ";
            if (!osg::equivalent(scale.length2(), 3.0f)) std::cout << "S = " << scale << ", ";
            if (!rot.zeroRotation()) std::cout << "R = " << euler << " (HPR), ";
            std::cout << "T = " << pos << std::endl;
        }

        if (!matrix.isIdentity())
        {
            osg::Vec3 pos, scale; osg::Quat rot, so; matrix.decompose(pos, rot, scale, so);
            osg::Vec3d euler = osgVerse::computeHPRFromQuat(rot);
            std::cout << std::string(_indent, ' ') << "<World>: ";
            if (!osg::equivalent(scale.length2(), 3.0f)) std::cout << "S = " << scale << ", ";
            if (!rot.zeroRotation()) std::cout << "R = " << euler << " (HPR), ";
            std::cout << "T = " << pos << std::endl;
        }

        pushMatrix(matrix);
        traverse(node);
        popMatrix(); popIndent();
    }

    virtual void apply(osg::Geode& node)
    {
        outputBasic(node); pushIndent();
        if (node.getStateSet()) apply(&node, NULL, *node.getStateSet());
#if OSG_VERSION_LESS_OR_EQUAL(3, 4, 1)
        for (unsigned int i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (geom != NULL) apply(*geom);
        }
#endif
        traverse(node); popIndent();
    }

    virtual void apply(osg::Geometry& geom)
    {
        osg::Matrix matrix;
        if (_matrixStack.size() > 0) matrix = _matrixStack.back();
        outputBasic(geom); pushIndent();

        osg::StateSet* ss = geom.getStateSet();
        if (ss) apply(geom.getNumParents() > 0 ? geom.getParent(0) : NULL, &geom, *ss);

        osgVerse::BoundingVolumeVisitor bvv; bvv.apply(geom);
        osgVerse::BoundingVolumeVisitor bvv1; bvv1.setWeldingVertices(true); bvv1.apply(geom);
        std::string nonManiType = "";
        switch (bvv1.isManifold())
        {
        case osgVerse::MeshCollector::IS_MANIFOLD: nonManiType = "Manifold"; break;
        case osgVerse::MeshCollector::NONMANIFOLD_EDGE: nonManiType = "Non-Manifold Edges"; break;
        case osgVerse::MeshCollector::UNCLOSED_MESH: nonManiType = "Unclosed Mesh"; break;
        case osgVerse::MeshCollector::SELF_INTERSECTION: nonManiType = "Self-Intersected Mesh"; break;
        case osgVerse::MeshCollector::NEGATIVE_VOLUME: nonManiType = "Negative Mesh"; break;
        default: break;
        }
        std::cout << std::string(_indent, ' ') << "<" << nonManiType << ">: "
                  << "Primitives = " << geom.getNumPrimitiveSets() << ", Triangles = "
                  << (bvv.getTriangles().size() / 3) << "; V = " << bvv.getVertices().size() << ",";
        if (!bvv.getAttributes(osgVerse::MeshCollector::NormalAttr).empty()) std::cout << " +N";
        if (!bvv.getAttributes(osgVerse::MeshCollector::ColorAttr).empty()) std::cout << " +C";
        for (size_t u = 0; u < geom.getNumTexCoordArrays(); ++u) std::cout << " +UV" << u;
        std::cout << "; WeldedV = " << bvv1.getVertices().size() << std::endl;
#if OSG_VERSION_GREATER_THAN(3, 4, 1)
        traverse(geom);
#endif
        popIndent();
    }

    virtual void apply(osg::Node* n, osg::Drawable* d, osg::StateSet& ss)
    {
        std::cout << std::string(_indent, ' ') << "<StateSet> " << ss.getName();
        if (ss.referenceCount() == 1) std::cout << std::endl;
        else std::cout << " (SHARED x" << ss.referenceCount() << ")" << std::endl;
        pushIndent();

        osg::StateSet::AttributeList& attrList = ss.getAttributeList();
        for (osg::StateSet::AttributeList::iterator itr = attrList.begin();
             itr != attrList.end(); ++itr)
        {
            osg::StateAttribute::Type t = itr->first.first;
            std::cout << std::string(_indent, ' ') << "<" << itr->second.first->className()
                      << "> " << itr->second.first->getName() << std::endl;
        }

        osg::StateSet::TextureAttributeList& texAttrList = ss.getTextureAttributeList();
        for (size_t i = 0; i < texAttrList.size(); ++i)
        {
            osg::StateSet::AttributeList& attr = texAttrList[i];
            for (osg::StateSet::AttributeList::iterator itr = attr.begin();
                itr != attr.end(); ++itr)
            {
                osg::StateAttribute::Type t = itr->first.first;
                std::cout << std::string(_indent, ' ') << "<" << itr->second.first->className()
                          << ", Unit-" << i << "> " << itr->second.first->getName();
                if (t != osg::StateAttribute::TEXTURE) { std::cout << std::endl; continue; }
                
                osg::Texture* tex = static_cast<osg::Texture*>(itr->second.first.get());
                if (tex->referenceCount() == 1) std::cout << std::endl;
                else std::cout << " (SHARED x" << tex->referenceCount() << ")" << std::endl;

                pushIndent();
                for (size_t k = 0; k < tex->getNumImages(); ++k)
                {
                    osg::Image* image = tex->getImage(k);
                    std::cout << std::string(_indent, ' ') << "<" << image->className() <<"> "
                              << image->getName() << ": Resolution = " << image->s() << "x" << image->t()
                              << "x" << image->r() << ", DDS = " <<image->isCompressed()
                              << ", Translucent = " << image->isImageTranslucent();
                    if (!image->getFileName().empty()) std::cout << ", " << image->getFileName();
                    if (image->referenceCount() == 1) std::cout << std::endl;
                    else std::cout << " (SHARED x" << image->referenceCount() << ")" << std::endl;
                }
                popIndent();
            }
        }
        popIndent();
    }

protected:
    inline void pushIndent() { _indent += 2; }
    inline void popIndent() { if (_indent > 0) _indent -= 2; }
    inline void pushMatrix(osg::Matrix& matrix) { _matrixStack.push_back(matrix); }
    inline void popMatrix() { _matrixStack.pop_back(); }

    inline void outputBasic(osg::Object& obj)
    {
        std::cout << std::string(_indent, ' ') << "[" << obj.libraryName() << "::"
                  << obj.className() << "] " << obj.getName() << ": Depth = " << (_indent / 2);
        osg::Geode* geode = (obj.asNode() != NULL) ? obj.asNode()->asGeode() : NULL;
        if (geode)
            std::cout << ", Drawables = " << geode->getNumDrawables();
        else
        {
            osg::Group* group = (obj.asNode() != NULL) ? obj.asNode()->asGroup() : NULL;
            if (group) std::cout << ", Children = " << group->getNumChildren();
        }

        if (obj.referenceCount() == 1) std::cout << std::endl;
        else std::cout << " (SHARED x" << obj.referenceCount() << ")" << std::endl;
    }

    typedef std::vector<osg::Matrix> MatrixStack;
    MatrixStack _matrixStack;
    int _indent;
};

int main(int argc, char** argv)
{
    osgVerse::globalInitialize(argc, argv);
    osgVerse::fixOsgBinaryWrappers();

    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    Reporter reporter;
    scene->accept(reporter);

    // Start the main loop
    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(scene.get());
    return viewer.run();
}
