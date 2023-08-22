#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/ImageSequence>
#include <osg/ImageUtils>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <pipeline/Utilities.h>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

int main(int argc, char** argv)
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <image/image_seq file>" << std::endl;
        return 1;
    }
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile(argv[1]);
    if (!image) return 1;

    // Try to get 3D image data (e.g., from a Tiff file)
    osg::ImageSequence* seq = dynamic_cast<osg::ImageSequence*>(image.get());
    if (seq != NULL)
    {
        osg::ImageList images;
#if OSG_VERSION_GREATER_THAN(3, 3, 0)
        osg::ImageSequence::ImageDataList imgList = seq->getImageDataList();
        for (size_t i = 0; i < imgList.size(); ++i) images.push_back(imgList[i]._image);
#else
        images = seq->getImages();
#endif
        if (!images.empty())
        {
            osg::Image* img0 = images[0].get();
            image = new osg::Image;
            image->allocateImage(img0->s(), img0->t(), images.size(),
                                 img0->getPixelFormat(), img0->getDataType());
            image->setInternalTextureFormat(img0->getInternalTextureFormat());
            for (size_t i = 0; i < images.size(); ++i)
            {
                osg::Image* imgN = images[i].get();
                osg::copyImage(imgN, 0, 0, 0, imgN->s(), imgN->t(), 1, image.get(), 0, 0, i, false);
            }
        }
    }

    // Create the 3D texture cube
    osg::ref_ptr<osg::Vec3Array> va = new osg::Vec3Array(8);
    osg::ref_ptr<osg::Vec3Array> ta = new osg::Vec3Array(8);
    (*va)[0] = osg::Vec3(0.0f, 0.0f, 0.0f); (*ta)[0] = osg::Vec3(0.0f, 0.0f, 0.0f);
    (*va)[1] = osg::Vec3(1.0f, 0.0f, 0.0f); (*ta)[1] = osg::Vec3(1.0f, 0.0f, 0.0f);
    (*va)[2] = osg::Vec3(1.0f, 1.0f, 0.0f); (*ta)[2] = osg::Vec3(1.0f, 1.0f, 0.0f);
    (*va)[3] = osg::Vec3(0.0f, 1.0f, 0.0f); (*ta)[3] = osg::Vec3(0.0f, 1.0f, 0.0f);
    (*va)[4] = osg::Vec3(0.0f, 0.0f, 1.0f); (*ta)[4] = osg::Vec3(0.0f, 0.0f, 1.0f);
    (*va)[5] = osg::Vec3(1.0f, 0.0f, 1.0f); (*ta)[5] = osg::Vec3(1.0f, 0.0f, 1.0f);
    (*va)[6] = osg::Vec3(1.0f, 1.0f, 1.0f); (*ta)[6] = osg::Vec3(1.0f, 1.0f, 1.0f);
    (*va)[7] = osg::Vec3(0.0f, 1.0f, 1.0f); (*ta)[7] = osg::Vec3(0.0f, 1.0f, 1.0f);

    osg::ref_ptr<osg::DrawElementsUByte> de = new osg::DrawElementsUByte(GL_QUADS);
    de->push_back(0); de->push_back(3); de->push_back(2); de->push_back(1);
    de->push_back(4); de->push_back(5); de->push_back(6); de->push_back(7);
    de->push_back(0); de->push_back(1); de->push_back(5); de->push_back(4);
    de->push_back(1); de->push_back(2); de->push_back(6); de->push_back(5);
    de->push_back(2); de->push_back(3); de->push_back(7); de->push_back(6);
    de->push_back(3); de->push_back(0); de->push_back(4); de->push_back(7);

    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
    geom->setUseDisplayList(false);
    geom->setUseVertexBufferObjects(true);
    geom->setVertexArray(va.get());
    geom->setTexCoordArray(0, ta.get());
    geom->addPrimitiveSet(de.get());
    geom->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::ref_ptr<osg::Texture3D> tex3D = new osg::Texture3D;
    tex3D->setImage(image.get());
    tex3D->setResizeNonPowerOfTwoHint(false);
    tex3D->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
    tex3D->setWrap(osg::Texture::WRAP_R, osg::Texture::CLAMP_TO_EDGE);
    geom->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex3D.get());

    // Interactive handler
    osgVerse::QuickEventHandler* handler = new osgVerse::QuickEventHandler;
    osg::Vec2 xOffset(0.0f, 1.0f), yOffset(0.0f, 1.0f), zOffset(0.0f, 1.0f);

    handler->addKeyDownCallback(new int[]{ 'a', 'A' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = {0, 3, 4, 7};
        if (key == 'a' && xOffset[0] < xOffset[1]) xOffset[0] += 0.01f;
        else if (key == 'A' && xOffset[0] > 0.0f) xOffset[0] -= 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].x() = xOffset[0]; (*ta)[ids[i]].x() = xOffset[0]; }
    });

    handler->addKeyDownCallback(new int[] { 'd', 'D' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = { 1, 2, 5, 6 };
        if (key == 'd' && xOffset[0] < xOffset[1]) xOffset[1] -= 0.01f;
        else if (key == 'D' && xOffset[1] < 1.0f) xOffset[1] += 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].x() = xOffset[1]; (*ta)[ids[i]].x() = xOffset[1]; }
    });

    handler->addKeyDownCallback(new int[] { 's', 'S' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = { 0, 1, 4, 5 };
        if (key == 's' && yOffset[0] < yOffset[1]) yOffset[0] += 0.01f;
        else if (key == 'S' && yOffset[0] > 0.0f) yOffset[0] -= 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].y() = yOffset[0]; (*ta)[ids[i]].y() = yOffset[0]; }
    });

    handler->addKeyDownCallback(new int[] { 'w', 'W' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = { 2, 3, 6, 7 };
        if (key == 'w' && yOffset[0] < yOffset[1]) yOffset[1] -= 0.01f;
        else if (key == 'W' && yOffset[1] < 1.0f) yOffset[1] += 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].y() = yOffset[1]; (*ta)[ids[i]].y() = yOffset[1]; }
    });

    handler->addKeyDownCallback(new int[] { 'q', 'Q' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = { 0, 1, 2, 3 };
        if (key == 'q' && zOffset[0] < zOffset[1]) zOffset[0] += 0.01f;
        else if (key == 'Q' && zOffset[0] > 0.0f) zOffset[0] -= 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].z() = zOffset[0]; (*ta)[ids[i]].z() = zOffset[0]; }
    });

    handler->addKeyDownCallback(new int[] { 'e', 'E' }, 2, [&](int key) {
        va->dirty(); ta->dirty(); int ids[4] = { 4, 5, 6, 7 };
        if (key == 'e' && zOffset[0] < zOffset[1]) zOffset[1] -= 0.01f;
        else if (key == 'E' && zOffset[1] < 1.0f) zOffset[1] += 0.01f;
        for (int i = 0; i < 4; ++i) { (*va)[ids[i]].z() = zOffset[1]; (*ta)[ids[i]].z() = zOffset[1]; }
    });

    // Add to scene graph and start the viewer
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(geom.get());

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(geode.get());
    root->addChild(osgDB::readNodeFile("axes.osgt"));

    osgViewer::Viewer viewer;
    viewer.addEventHandler(handler);
    //viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
