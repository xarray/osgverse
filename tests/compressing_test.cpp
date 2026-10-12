#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include <VerseCommon.h>
#include <ui/ImGui.h>
#include <ui/Utilities.h>
#include <modeling/Utilities.h>
#include <readerwriter/Utilities.h>
#include <readerwriter/TextureSizeAdvisor.h>
#include <readerwriter/DatabasePager.h>
#include <readerwriter/DracoProcessor.h>
#ifdef OSG_LIBRARY_STATIC
USE_SERIALIZER_WRAPPER(DracoGeometry)
#endif

#ifndef _DEBUG
#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }
#endif

class SceneDataOptimizer : public osgVerse::TextureOptimizer
{
public:
    SceneDataOptimizer()
        : osgVerse::TextureOptimizer(true, "optimize_tex_compress_test") {}

protected:
    virtual void applyTexture(osg::Texture* tex, unsigned int unit)
    {
        osg::Image* image = tex->getImage(0);
        if (image && image->valid() && !image->isMipmap())
        {
            image->ensureValidSizeForTexturing(2048);
            osgVerse::generateMipmaps(*image, false);
        }

        osgVerse::TextureOptimizer::applyTexture(tex, unit);
        tex->setClientStorageHint(false);
        tex->setUnRefImageDataAfterApply(true);
    }
};

// Create a synthetic test image of the given kind, used to check the texture size advisor
static osg::Image* makeTestImage(int w, int h, const std::string& kind)
{
    osg::ref_ptr<osg::Image> img = new osg::Image;
    img->allocateImage(w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE);
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            unsigned char* p = img->data(x, y);
            if (kind == "solid") { p[0] = p[1] = p[2] = 200; p[3] = 255; }
            else if (kind == "gradient") { p[0] = (unsigned char)(x * 255 / w); p[1] = (unsigned char)(y * 255 / h); p[2] = 96; p[3] = 255; }
            else if (kind == "checker") { unsigned char c = ((x / 16 + y / 16) % 2) ? 255 : 0; p[0] = p[1] = p[2] = c; p[3] = 255; }
            else { p[0] = (unsigned char)(rand() & 0xff); p[1] = (unsigned char)(rand() & 0xff); p[2] = (unsigned char)(rand() & 0xff); p[3] = 255; }
        }
    return img.release();
}

// Ad-hoc test of TextureSizeAdvisor. Oversized textures are trimmed down to the on-screen
// reference size; smooth images (solid/gradient) shrink far below it while detailed ones
// (checker/noise) keep the screen resolution. Suggested sizes must keep the aspect ratio,
// stay power-of-two for power-of-two inputs, and be aligned to the compression block size.
static bool isPowerOfTwo(int x) { return x > 0 && (x & (x - 1)) == 0; }

static void testTextureSizeAdvisor()
{
    osgVerse::TextureSizeAdvisor::Options opt;  // targetScreenSize = 1000 by default
    std::cout << "[TextureSizeAdvisor] target screen size: " << opt.targetScreenSize << "\n";
    int screenSize = opt.targetScreenSize, failures = 0;

    struct TestCase { const char* kind; int w, h; bool smooth, potInput; };
    TestCase cases[] = {
        { "solid",  2048, 2048, true,  true  },
        { "gradient", 2048, 2048, true,  true  },
        { "checker", 2048, 2048, false, true  },
        { "noise",  2048, 2048, false, true  },
        { "noise",  2048, 1024, false, true  },  // 2:1 aspect, both power of two
        { "noise",  1600, 1200, false, false },  // 4:3 aspect, not power of two
    };

    for (size_t i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
    {
        const TestCase& c = cases[i];
        osg::ref_ptr<osg::Image> img = makeTestImage(c.w, c.h, c.kind);
        osgVerse::TextureSizeAdvisor::Result result = osgVerse::TextureSizeAdvisor::analyze(*img, opt);
        std::cout << "  " << c.kind << " " << c.w << "x" << c.h << ": "
                  << osgVerse::TextureSizeAdvisor::toString(result) << "\n";

        bool ok = result.reduced && result.suggestedWidth > 0 && result.suggestedHeight > 0;
        double srcAspect = (double)c.w / c.h;
        double dstAspect = (double)result.suggestedWidth / result.suggestedHeight;
        ok = ok && (std::fabs(srcAspect - dstAspect) < 0.02);                        // keep aspect
        ok = ok && (result.suggestedWidth % opt.blockAlign == 0)                    // valid for compression
                && (result.suggestedHeight % opt.blockAlign == 0);
        if (c.potInput)                                                             // power-of-two stays power-of-two
            ok = ok && isPowerOfTwo(result.suggestedWidth) && isPowerOfTwo(result.suggestedHeight);
        if (c.smooth)
            ok = ok && (result.suggestedWidth <= 64 && result.suggestedHeight <= 64);
        else
            ok = ok && (std::max(result.suggestedWidth, result.suggestedHeight) >= screenSize * 9 / 10);
        if (!ok) { failures++; std::cout << "    [Error] unexpected suggestion\n"; }
    }

    // Also check the in-place helper: a solid 4096 image should become tiny and still power-of-two
    osg::ref_ptr<osg::Image> solid = makeTestImage(4096, 4096, "solid");
    bool resized = osgVerse::TextureSizeAdvisor::optimizeSize(*solid, opt);
    std::cout << "  optimizeSize(solid 4096): resized=" << resized
              << ", now=" << solid->s() << "x" << solid->t() << "\n";
    if (!resized || solid->s() > 64 || !isPowerOfTwo(solid->s())) failures++;

    std::cout << "[TextureSizeAdvisor] test " << (failures ? "FAILED" : "PASSED")
              << " (" << failures << " unexpected result(s))\n";
}

// Check that TextureOptimizer really shrinks textures when the option is enabled
static void testTextureOptimizerIntegration()
{
    osg::ref_ptr<osg::Image> img = makeTestImage(2048, 2048, "solid");
    osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D(img.get());
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex.get());

    osgVerse::TextureOptimizer opt(true, "optimize_tex_test");
    opt.setAutoResizingTextures(true);
    geode->accept(opt);
    opt.deleteSavedTextures();

    bool ok = (img->s() <= 64 && img->t() <= 64);
    std::cout << "  [TextureOptimizer] auto-resize: image now " << img->s() << "x" << img->t()
              << (ok ? " (OK)" : " (unexpected)") << "\n";
}

int main(int argc, char** argv)
{
    osg::ArgumentParser arguments = osgVerse::globalInitialize(argc, argv, osgVerse::defaultInitParameters());
    osgVerse::updateOsgBinaryWrappers();
    if (arguments.read("--advisor")) { testTextureSizeAdvisor(); testTextureOptimizerIntegration(); return 0; }

    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;
    osgViewer::Viewer viewer;

#if true
    osg::ref_ptr<osg::Image> image = osgDB::readImageFile("Images/osg256.png");
    //osgVerse::AudioPlayer::instance()->addFile("../voice.wav", true, false);
    //osgVerse::AudioPlayer::instance()->addFile("../sample.wav", true, true);

    osg::Timer_t t0 = osg::Timer::instance()->tick();
    osg::ref_ptr<osg::Image> dds = osgVerse::compressImage(*image);

    osg::Timer_t t1 = osg::Timer::instance()->tick();
    osgVerse::resizeImage(*dds, 128, 128);

    osg::Timer_t t2 = osg::Timer::instance()->tick();
    std::cout << "Compressing time: " << osg::Timer::instance()->delta_m(t0, t1) << "\n"
              << "Resizing time: " << osg::Timer::instance()->delta_m(t1, t2) << "\n";

    root->addChild(osg::createGeodeForImage(dds.get()));
    root->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
#elif false
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFiles(arguments);
    if (node)
    {
        osgVerse::MeshCollector mc; node->accept(mc);
        std::vector<osg::ref_ptr<osg::Geometry>> geomList = mc.output();

        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        for (size_t i = 0; i < geomList.size(); ++i)
        {
            osgVerse::MeshOptimizer optimizer;
            optimizer.optimize(geomList[i].get());
            geode->addDrawable(geomList[i].get());
        }
        root->addChild(geode.get());
    }
#elif false
    std::string outFile = "result.osgb";
    if (arguments.read("--out"))
    {
        osg::ref_ptr<osg::Node> node = osgDB::readNodeFiles(arguments);
        if (node)
        {
            SceneDataOptimizer sdo; node->accept(sdo);
            sdo.deleteSavedTextures();

            osg::ref_ptr<osgDB::Options> options = new osgDB::Options("WriteImageHint=IncludeFile");
            options->setPluginStringData("UseBASISU", "1");
            arguments.read("--filename", outFile);
            osgDB::writeNodeFile(*node, outFile, options.get());
            root->addChild(node.get());
        }
    }
#else
    root->addChild(osgDB::readNodeFiles(arguments));
    root->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    viewer.setDatabasePager(new osgVerse::DatabasePager(true));
#endif

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    //viewer.setUpViewInWindow(0, 0, 1920, 1080);
    viewer.realize();

    while (!viewer.done())
    {
        //if (viewer.getFrameStamp()->getFrameNumber() == 5)
        //    osgVerse::FileDialog::selectFolder("Hello");
        viewer.frame();
    }
    return 0;
}
