#include <osg/io_utils>
#include <osg/LightSource>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgGA/TrackballManipulator>
#include <osgUtil/CullVisitor>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <pipeline/Pipeline.h>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

#define TEST_DEFERRED_RTT 0
#define TEST_PBO_UPDATING 1

int main(int argc, char** argv)
{
    osgViewer::Viewer viewer;
    osg::ref_ptr<osg::MatrixTransform> root = new osg::MatrixTransform;

#if TEST_DEFERRED_RTT
    osg::ref_ptr<osg::Node> scene =
        (argc < 2) ? osgDB::readNodeFile("cessna.osg") : osgDB::readNodeFile(argv[1]);
    if (!scene) { OSG_WARN << "Failed to load " << (argc < 2) ? "" : argv[1]; return 1; }

    // The scene graph
    osg::ref_ptr<osg::MatrixTransform> sceneRoot = new osg::MatrixTransform;
    sceneRoot->addChild(scene.get());

    osg::ref_ptr<osg::Node> otherSceneRoot = osgDB::readNodeFile(
        "skydome.osgt.(0.005,0.005,0.01).scale.-100,-150,0.trans");
    root->addChild(otherSceneRoot.get());
    root->addChild(sceneRoot.get());

    osg::ref_ptr<osg::Texture> tex2D_0 =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGB_INT8, 1024, 1024);
    osg::ref_ptr<osg::Texture> tex2D_1 =
        osgVerse::Pipeline::createTexture(osgVerse::Pipeline::RGB_INT8, 1024, 1024);
    otherSceneRoot->getOrCreateStateSet()->setTextureAttributeAndModes(
        0, tex2D_1.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);

    osg::ref_ptr<osgVerse::DeferredRenderCallback> drcb = new osgVerse::DeferredRenderCallback(false);
    {
        osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> gr0 =
            new osgVerse::DeferredRenderCallback::RttGeometryRunner("Test0");
        gr0->projection = osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0);
        gr0->geometry = osg::createTexturedQuadGeometry(
            osg::Vec3(0.2f, 0.2f, 0.0f), osg::Vec3(0.6f, 0.0f, 0.0f), osg::Vec3(0.0f, 0.6f, 0.0f));
        {
            osg::ref_ptr<osg::Texture2D> osg128 = new osg::Texture2D;
            osg128->setImage(osgDB::readImageFile("Images/osg128.png"));
            gr0->geometry->getOrCreateStateSet()->setTextureAttributeAndModes(0, osg128.get());
            gr0->geometry->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            gr0->geometry->getOrCreateStateSet()->setMode(GL_BLEND, osg::StateAttribute::ON);
        }
        gr0->attach(osg::Camera::COLOR_BUFFER, tex2D_0.get());
        drcb->addRunner(gr0.get());

        osg::ref_ptr<osgVerse::DeferredRenderCallback::RttGeometryRunner> gr1 =
            new osgVerse::DeferredRenderCallback::RttGeometryRunner("Test1");
        gr1->setUseScreenQuad(0, tex2D_0.get());
        gr1->attach(osg::Camera::COLOR_BUFFER, tex2D_1.get());
        {
            const char* testFrag = {
                "uniform sampler2D MainTex;\n"
                "varying vec4 gl_TexCoord[gl_MaxTextureCoords];\n"
                "void main() {\n"
                "  if (gl_FragCoord.x < 512) gl_FragColor = texture2D(MainTex, gl_TexCoord[0].st);\n"
                "  else gl_FragColor = texture2D(MainTex, gl_TexCoord[0].st).bgra;\n"
                "}\n"
            };

            osg::ref_ptr<osg::Program> program = new osg::Program;
            program->addShader(new osg::Shader(osg::Shader::FRAGMENT, testFrag));
            gr1->geometry->getOrCreateStateSet()->setAttributeAndModes(program.get());
            gr1->geometry->getOrCreateStateSet()->addUniform(new osg::Uniform("MainTex", (int)0));
        }
        drcb->addRunner(gr1.get());
    }
    drcb->setup(viewer.getCamera(), PRE_DRAW);
#elif TEST_PBO_UPDATING
    class TextureUpdater : public osg::NodeCallback
    {
    public:
        virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
        {
            osg::Texture2D* tex = static_cast<osg::Texture2D*>(
                node->getOrCreateStateSet()->getTextureAttribute(0, osg::StateAttribute::TEXTURE));
            if (!tex)
            {
                tex = new osg::Texture2D;
                tex->setImage(new osg::Image);
                tex->setFilter(osg::Texture2D::MIN_FILTER, osg::Texture2D::LINEAR);
                tex->setFilter(osg::Texture2D::MAG_FILTER, osg::Texture2D::LINEAR);
                node->getOrCreateStateSet()->setTextureAttributeAndModes(0, tex);
                updateImage(tex, tex->getImage(), true);
            }
            else
                updateImage(tex, tex->getImage(), false);
            traverse(node, nv);
        }

        void updateImage(osg::Texture2D* tex, osg::Image* img, bool created)
        {
            if (created)
            {
                img->allocateImage(4096, 4096, 1, GL_RGBA, GL_UNSIGNED_BYTE);
                img->setPixelBufferObject(new osg::PixelBufferObject(img));
                img->setDataVariance(DYNAMIC); _value = 0;
            }

            if (_value > 255) _value = 0; else _value++;
            memset(img->data(), _value, img->getTotalSizeInBytes());
            img->dirty();
        }

    protected:
        int _value;
    };

    osg::Geometry* geom = osg::createTexturedQuadGeometry(osg::Vec3(), osg::X_AXIS, osg::Z_AXIS);
    geom->setUpdateCallback(new TextureUpdater);

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(geom);
    root->addChild(geode);

    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
#endif

    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    return viewer.run();
}
