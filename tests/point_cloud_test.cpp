#include <osg/io_utils>
#include <osg/MatrixTransform>
#include <osg/PagedLOD>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileNameUtils>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgText/Text>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <iostream>
#include <sstream>

#include <backward.hpp>  // for better debug info
namespace backward { backward::SignalHandling sh; }

class PointSelectorCallback : public osg::Camera::DrawCallback
{
public:
    virtual void operator()(osg::RenderInfo& renderInfo) const
    {
        unsigned char pixels[4]; float depth = 0;
        glReadPixels(_screenCoord[0], _screenCoord[1], 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        glReadPixels(_screenCoord[0], _screenCoord[1], 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
        if (depth < 1.0f)
        {
            osg::Vec4 viewCoord(_screenCoord[0], _screenCoord[1], depth, 1.0);
            osg::Matrix mvpw = renderInfo.getCurrentCamera()->getViewMatrix()
                             * renderInfo.getCurrentCamera()->getProjectionMatrix()
                             * renderInfo.getCurrentCamera()->getViewport()->computeWindowMatrix();
            _worldCoord = viewCoord * osg::Matrix::inverse(mvpw);
            _worldCoord /= _worldCoord.w();
            _color = osg::Vec4(pixels[0] / 255.0f, pixels[1] / 255.0f,
                               pixels[2] / 255.0f, pixels[3] / 255.0f);
        }
    }

    void setScreenCoord(const osg::Vec2& c) { _screenCoord = c; }
    const osg::Vec4& getCurrentWorldCoord() const { return _worldCoord; }
    const osg::Vec4& getCurrentColor() const { return _color; }

protected:
    mutable osg::Vec4 _worldCoord, _color;
    osg::Vec2 _screenCoord;
};

class PointSelector : public osgGA::GUIEventHandler
{
public:
    PointSelector(osg::Camera* cam, osgText::Text* text)
    {
        _text = text;
        _callback = new PointSelectorCallback;
        cam->setPostDrawCallback(_callback.get());
    }
    
    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::MOVE:
            _callback->setScreenCoord(osg::Vec2(ea.getX(), ea.getY()));
            break;
        case osgGA::GUIEventAdapter::FRAME:
            {
                const osg::Vec4& w = _callback->getCurrentWorldCoord();
                const osg::Vec4& c = _callback->getCurrentColor();
                
                static char message[256];
                sprintf(message, "Coord: %lg, %lg, %lg; Color: %lg, %lg, %lg",
                        w[0], w[1], w[2], c[0], c[1], c[2]);
                _text->setText(message);
            }
            break;
        }
        return false;
    }

protected:
    osg::observer_ptr<PointSelectorCallback> _callback;
    osg::observer_ptr<osgText::Text> _text;
};

osg::Camera* createHUD(osgText::Text* text, unsigned int w, unsigned int h)
{
    osg::Camera* camera = new osg::Camera;
    camera->setProjectionMatrix(osg::Matrix::ortho2D(0, w, 0, h));
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setViewMatrix(osg::Matrix::identity());
    camera->setClearMask(GL_DEPTH_BUFFER_BIT);
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setAllowEventFocus(false);

    osg::Geode* geode = new osg::Geode;
    geode->addDrawable(text);
    camera->addChild(geode);

    osg::Vec3 position(50.0f, h - 50, 0.0f);
    text->setPosition(position);
    text->setText("A simple multi-touch-example");
    return camera;
}

int main(int argc, char** argv)
{
    std::string filename = argc > 1 ? argv[1] : "";
    std::string ext = osgDB::getFileExtension(filename);
    if (ext != "verse_ept") filename += ".verse_ept";

    osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(filename);
    if (!scene) { OSG_WARN << "Failed to load EPT point cloud"; return 1; }

    osg::ref_ptr<osgText::Text> text = new osgText::Text;
    text->setFont("fonts/arial.ttf");

    osg::ref_ptr<osg::Group> root = new osg::Group;
    root->addChild(scene.get());
    root->addChild(createHUD(text.get(), 1920, 1080));

    osg::StateSet* stateset = root->getOrCreateStateSet();
    stateset->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osgViewer::Viewer viewer;
    viewer.addEventHandler(new PointSelector(viewer.getCamera(), text.get()));
    viewer.addEventHandler(new osgViewer::StatsHandler);
    viewer.addEventHandler(new osgViewer::WindowSizeHandler);
    viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getStateSet()));
    viewer.setCameraManipulator(new osgGA::TrackballManipulator);
    viewer.setSceneData(root.get());
    viewer.setUpViewOnSingleScreen(0);
    return viewer.run();
}
