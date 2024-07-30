#include <osg/io_utils>
#include <osg/Multisample>
#include <osg/Texture2D>
#include <osg/LOD>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>

#include <modeling/Math.h>
#include <modeling/Utilities.h>
#include <pipeline/IntersectionManager.h>
#include <pipeline/Utilities.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class ApplyTexCoordVisitor : public osg::NodeVisitor
{
public:
    ApplyTexCoordVisitor(int u = 1)
    :   osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _unit(u) {}
    inline void pushMatrix(const osg::Matrix& matrix) { _matrixStack.push_back(matrix); }
    inline void popMatrix() { _matrixStack.pop_back(); }

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
        if (_matrixStack.size() > 0) matrix = _matrixStack.back();
        for (size_t i = 0; i < node.getNumDrawables(); ++i)
        {
            osg::Geometry* geom = node.getDrawable(i)->asGeometry();
            if (!geom) continue;

            osg::Vec3Array* va = static_cast<osg::Vec3Array*>(geom->getVertexArray());
            osg::Vec2Array* ta = new osg::Vec2Array;
            for (size_t v = 0; v < va->size(); ++v)
            {
                osg::Vec3 pt = (*va)[v] * matrix;
                ta->push_back(osg::Vec2(pt[0], pt[1]));
            }
            geom->setTexCoordArray(_unit, ta);
        }
        traverse(node);
    }

protected:
    typedef std::vector<osg::Matrix> MatrixStack;
    MatrixStack _matrixStack; int _unit;
};

void createBlendingArea(osg::Node* input, const osg::Vec3& dir, const osg::Vec3& up,
                        float offset = 0.1f, int texUnit = 1)
{
    osgVerse::MeshCollector mc; mc.setWeldingVertices(true);
    mc.setUseGlobalVertices(true); input->accept(mc);
    const std::vector<osg::Vec3> vertices = mc.getVertices();
    const std::vector<unsigned int> triangles = mc.getTriangles();

    // 1. Find boundary edges, which only have 1 triangle attached
    std::map<osgVerse::EdgeType, int> edgeMap;
    for (size_t i = 0; i < triangles.size(); i += 3)
    {
        unsigned int i0 = triangles[i], i1 = triangles[i + 1], i2 = triangles[i + 2];
        osgVerse::EdgeType e0(osg::minimum(i0, i1), osg::maximum(i0, i1));
        osgVerse::EdgeType e1(osg::minimum(i0, i2), osg::maximum(i0, i2));
        osgVerse::EdgeType e2(osg::minimum(i1, i2), osg::maximum(i1, i2));
        edgeMap[e0]++; edgeMap[e1]++; edgeMap[e2]++;
    }

    std::set<unsigned int> boundary;
    for (std::map<osgVerse::EdgeType, int>::iterator itr = edgeMap.begin();
         itr != edgeMap.end(); ++itr)
    {
        if (itr->second != 1) continue;
        boundary.insert(itr->first.first); boundary.insert(itr->first.second);
    }

    // 2. Project boundary points to 2D plane which matches the best view area
    osgVerse::PointList2D points2; osgVerse::PointList3D points3;
    for (std::set<unsigned int>::iterator itr = boundary.begin();
         itr != boundary.end(); ++itr) { points3.push_back(vertices[*itr]); }
    osg::Matrix mat3To2 = osgVerse::GeometryAlgorithm::project(points3, -dir, up, points2);
    osgVerse::GeometryAlgorithm::reorderPointsInPlane(points2);

    // 3. Apply 2D coord to each original vertex as UV1
    float radius = input->getBound().radius() * 1.5f;
    osg::Vec2d poi = osgVerse::GeometryAlgorithm::getPoleOfInaccessibility(points2);
    ApplyTexCoordVisitor atv(texUnit); atv.pushMatrix(mat3To2); input->accept(atv);

    // 4. Create 1D texture for 360deg distances from POI to boundary
    osg::ref_ptr<osg::Texture1D> tex = new osg::Texture1D;
    tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
    tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
    tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    {
        osg::ref_ptr<osg::Image> image = new osg::Image;
        image->allocateImage(256, 1, 1, GL_RGB, GL_FLOAT);
        image->setInternalTextureFormat(GL_RGB32F_ARB);
        for (int i = 0; i < 256; ++i)
        {
            float angle = osg::PI * 2.0f * (float)i / 256.0f;
            osg::Vec2 dir(cosf(angle), sinf(angle));
            osgVerse::PointList2D result = osgVerse::GeometryAlgorithm::intersectionWithPolygon2D(
                osgVerse::LineType2D(poi, poi + dir * radius), points2);
            if (result.empty())
                { OSG_WARN << "No intersection found at angle " << angle << std::endl; }
            else
                ((osg::Vec3f*)image->data(i, 0))->set((result.back().first - poi).length(), 0.0f, 0.0f);
        }
        tex->setImage(0, image.get());
    }

    // 5. Apply textures, uniforms, and shaders
    osg::StateSet* ss = input->getOrCreateStateSet();
    ss->addUniform(new osg::Uniform("poleOfInaccess", osg::Vec2f(poi)));
    ss->addUniform(new osg::Uniform("DistanceMap", texUnit));
    ss->setTextureAttributeAndModes(texUnit, tex.get());
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv);
    osg::Vec3 direction = osg::Y_AXIS;
    arguments.read("--dir-x", direction.x());
    arguments.read("--dir-y", direction.y());
    arguments.read("--dir-z", direction.z());

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFiles(arguments);
    if (!scene) { OSG_WARN << "Failed to load polygon file" << std::endl; return 1; }

    osg::Vec3 side, up = osg::Z_AXIS; direction.normalize();
    if (direction.z() > 0.99f || direction.z() < -0.99f) up = osg::Y_AXIS;
    side = up ^ direction; up = direction ^ side;

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(scene.get());
    createBlendingArea(scene.get(), direction, up, 0.1f);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
