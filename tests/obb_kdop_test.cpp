#include <osg/io_utils>
#include <osg/PolygonMode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <modeling/Utilities.h>
#include <iostream>
#include <sstream>

osg::Geometry* createOBB(osg::Node* node)
{
    osgVerse::BoundingVolumeVisitor bvv;
    node->accept(bvv);

    osg::Quat rotation;
    osg::BoundingBox bb = bvv.computeOBB(rotation);
    osg::Vec3 extent = bb._max - bb._min;

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(8);
    (*va)[0] = rotation * bb._min;
    (*va)[1] = rotation * (bb._min + osg::Vec3(extent[0], 0.0f, 0.0f));
    (*va)[2] = rotation * (bb._min + osg::Vec3(0.0f, extent[1], 0.0f));
    (*va)[3] = rotation * (bb._min + osg::Vec3(extent[0], extent[1], 0.0f));
    (*va)[4] = rotation * (bb._min + osg::Vec3(0.0f, 0.0f, extent[2]));
    (*va)[5] = rotation * (bb._min + osg::Vec3(extent[0], 0.0f, extent[2]));
    (*va)[6] = rotation * (bb._min + osg::Vec3(0.0f, extent[1], extent[2]));
    (*va)[7] = rotation * bb._max;

    osg::ref_ptr<osg::DrawElementsUByte> de = new osg::DrawElementsUByte(GL_QUADS);
    de->push_back(0); de->push_back(1); de->push_back(3); de->push_back(2);
    de->push_back(4); de->push_back(5); de->push_back(7); de->push_back(6);
    de->push_back(0); de->push_back(1); de->push_back(5); de->push_back(4);
    de->push_back(1); de->push_back(3); de->push_back(7); de->push_back(5);
    de->push_back(3); de->push_back(2); de->push_back(6); de->push_back(7);
    de->push_back(2); de->push_back(0); de->push_back(4); de->push_back(6);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(va.get());
    geom->addPrimitiveSet(de.get());
    return geom.release();
}

/*osg::Geometry* createKDop(osg::Node* node)
{
    osgVerse::BoundingVolumeVisitor bvv;
    node->accept(bvv);

    std::vector<osgVerse::ConvexHull> hulls;
    if (!bvv.computeKDop(hulls, 10)) return false;

    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array;
    osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_TRIANGLES);
    for (size_t i = 0; i < hulls.size(); ++i)
    {
        const osgVerse::ConvexHull& h = hulls[i];
        size_t indexOffset = va->size();
        for (size_t p = 0; p < h.points.size(); ++p)
            va->push_back(h.points[p]);
        for (size_t t = 0; t < h.triangles.size(); ++t)
            de->push_back(indexOffset + h.triangles[t]);
    }

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setVertexArray(va.get());
    geom->addPrimitiveSet(de.get());
    return geom.release();
}*/

int main(int argc, char** argv)
{
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("dumptruck.osgt") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    osg::ref_ptr<osg::MatrixTransform> cloned = new osg::MatrixTransform;
    cloned->setMatrix(osg::Matrix::rotate(osg::PI_4, osg::Z_AXIS) *
                      osg::Matrix::translate(50.0f, 0.0f, 0.0f));
    cloned->addChild(scene.get());

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet()->setAttributeAndModes(
        new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE));
    geode->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    //geode->addDrawable(createKDop(scene.get()));
    geode->addDrawable(createOBB(cloned.get()));

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    root->addChild(scene.get());
    root->addChild(cloned.get());
    root->addChild(geode.get());

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
