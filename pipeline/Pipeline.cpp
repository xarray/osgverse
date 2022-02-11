#include <osg/io_utils>
#include <osg/ValueObject>
#include <osgDB/ReadFile>
#include <osgUtil/RenderStage>
#include <osgViewer/Renderer>
#include <iostream>
#include <sstream>
#include <stdarg.h>
#include "Pipeline.h"
#include "Utilities.h"

struct MyClampProjectionCallback : public osg::CullSettings::ClampProjectionMatrixCallback
{
    template<class MatrixType>
    bool _clampProjectionMatrix(MatrixType& proj, double& znear, double& zfar) const
    {
        static double epsilon = 1e-6;
        osg::Vec2d nearFar = _callback->getCalculatedNearFar();
        if (nearFar[0] > 0.0 && nearFar[1] > 0.0)
        {
            if (fabs(proj(0, 3)) < epsilon  && fabs(proj(1, 3)) < epsilon  && fabs(proj(2, 3)) < epsilon)
            {   // Orthographic matrix
                proj(2, 2) = -2.0f / (nearFar[1] - nearFar[0]);
                proj(3, 2) = -(nearFar[1] + nearFar[0]) / (nearFar[1] - nearFar[0]);
            }
            else
            {   // Persepective matrix
                double tNear = (-nearFar[0] * proj(2, 2) + proj(3, 2))
                    / (-nearFar[0] * proj(2, 3) + proj(3, 3));
                double tFar = (-nearFar[1] * proj(2, 2) + proj(3, 2))
                    / (-nearFar[1] * proj(2, 3) + proj(3, 3));
                double ratio = fabs(2.0 / (tNear - tFar)), center = -(tNear + tFar) / 2.0;
                proj.postMult(osg::Matrix(1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, ratio, 0.0f,
                                          0.0f, 0.0f, center * ratio, 1.0f));
            }
        }
        znear = nearFar[0]; zfar = nearFar[1];
        return true;
    }

    virtual bool clampProjectionMatrixImplementation(osg::Matrixf& p, double& znear, double& zfar) const
    { return _clampProjectionMatrix(p, znear, zfar); }

    virtual bool clampProjectionMatrixImplementation(osg::Matrixd& p, double& znear, double& zfar) const
    { return _clampProjectionMatrix(p, znear, zfar); }

    MyClampProjectionCallback(osgVerse::DeferredRenderCallback* cb) : _callback(cb) {}
    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
};

class MySceneView : public osgUtil::SceneView
{
public:
    MySceneView(osgVerse::DeferredRenderCallback* cb, osg::DisplaySettings* ds = NULL)
        : osgUtil::SceneView(ds), _callback(cb) {}
    MySceneView(const MySceneView& sv, const osg::CopyOp& copyop = osg::CopyOp())
        : osgUtil::SceneView(sv, copyop), _callback(sv._callback) {}

    virtual void cull()
    {
        bool calcNearFar = false; getCamera()->getUserValue("NeedNearFarCalculation", calcNearFar);
        if (calcNearFar && _callback.valid()) _callback->cullWithNearFarCalculation(this);
        osgUtil::SceneView::cull();

        osg::FrameBufferObject* fbo = (getRenderStage() != NULL)
                                    ? getRenderStage()->getFrameBufferObject() : NULL;
        if (fbo && _callback.valid())
        {
            if (fbo->hasAttachment(osg::Camera::DEPTH_BUFFER))
                _callback->registerDepthFBO(getCamera(), fbo);
        }

        //double ratio = 0.0, fovy = 0.0, znear = 0.0, zfar = 0.0;
        //getProjectionMatrix().getPerspective(fovy, ratio, znear, zfar);
        //printf("%s[%d] %s: %lg, %lg\n", getName().c_str(), getFrameStamp()->getFrameNumber(),
        //       getCamera()->getName().c_str(), znear, zfar);
    }

protected:
    osg::observer_ptr<osgVerse::DeferredRenderCallback> _callback;
};

class MyRenderer : public osgViewer::Renderer
{
public:
    MyRenderer(osg::Camera* c) : osgViewer::Renderer(c) {}
    
    void useCustomSceneViews(osgVerse::DeferredRenderCallback* cb)
    {
        unsigned int opt = osgUtil::SceneView::HEADLIGHT;
        osgViewer::View* view = dynamic_cast<osgViewer::View*>(_camera->getView());
        if (view)
        {
            switch (view->getLightingMode())
            {
            case(osg::View::NO_LIGHT): opt = 0; break;
            case(osg::View::SKY_LIGHT): opt = osgUtil::SceneView::SKY_LIGHT; break;
            case(osg::View::HEADLIGHT): opt = osgUtil::SceneView::HEADLIGHT; break;
            }
        }

        osg::ref_ptr<osgUtil::SceneView> sceneView0 = useCustomSceneView(0, opt, cb);
        osg::ref_ptr<osgUtil::SceneView> sceneView1 = useCustomSceneView(1, opt, cb);
        _sceneView[0] = sceneView0; sceneView0->setName("SceneView0");
        _sceneView[1] = sceneView1; sceneView1->setName("SceneView1");
        _availableQueue._queue.clear();
        _availableQueue.add(_sceneView[0]);
        _availableQueue.add(_sceneView[1]);
    }

protected:
    osgUtil::SceneView* useCustomSceneView(unsigned int i, unsigned int flags,
                                           osgVerse::DeferredRenderCallback* cb)
    {
        osg::ref_ptr<osgUtil::SceneView> newSceneView = new MySceneView(cb);
        newSceneView->setFrameStamp(_sceneView[i]->getFrameStamp());
        newSceneView->setAutomaticFlush(_sceneView[i]->getAutomaticFlush());
        newSceneView->setGlobalStateSet(_sceneView[i]->getGlobalStateSet());
        newSceneView->setSecondaryStateSet(_sceneView[i]->getSecondaryStateSet());

        newSceneView->setDefaults(flags);
        if (_sceneView[i]->getDisplaySettings())
            newSceneView->setDisplaySettings(_sceneView[i]->getDisplaySettings());
        else
            newSceneView->setResetColorMaskToAllOn(false);
        newSceneView->setCamera(_camera.get(), false);

        newSceneView->setCullVisitor(_sceneView[i]->getCullVisitor());
        newSceneView->setCullVisitorLeft(_sceneView[i]->getCullVisitorLeft());
        newSceneView->setCullVisitorRight(_sceneView[i]->getCullVisitorRight());
        return newSceneView.release();
    }
};

namespace osgVerse
{
    static osg::GraphicsContext* createGraphicsContext(int w, int h, osg::GraphicsContext* shared = NULL)
    {
        osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
        traits->x = 0; traits->y = 0; traits->width = w; traits->height = h;
        traits->windowDecoration = false; traits->doubleBuffer = true;
        traits->sharedContext = shared; traits->vsync = false;
        return osg::GraphicsContext::createGraphicsContext(traits.get());
    }

    Pipeline::Pipeline()
    {
        _deferredCallback = new osgVerse::DeferredRenderCallback(true);
    }

    void Pipeline::Stage::applyUniform(const std::string& name, osg::Uniform* u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        if (ss->getUniform(name) == NULL) ss->addUniform(u);
    }

    void Pipeline::Stage::applyBuffer(Stage& src, const std::string& buffer, int unit)
    {
        if (src.outputs.find(buffer) != src.outputs.end())
        {
            osg::Texture2D* tex = src.outputs[buffer].get();
            osg::StateSet* ss = deferred ?
                runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
            ss->setTextureAttributeAndModes(unit, tex);
            ss->addUniform(new osg::Uniform(buffer.data(), unit));
        }
        else
            std::cout << buffer << " is undefined at stage " << name
                      << ", which sources from stage " << src.name << "\n";
    }

    void Pipeline::Stage::applyTexture(osg::Texture* tex, const std::string& buffer, int u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(u, tex);
        ss->addUniform(new osg::Uniform(buffer.data(), u));
    }

    void Pipeline::Stage::applyDefaultTexture(const osg::Vec4& color, const std::string& buffer, int u)
    {
        osg::StateSet* ss = deferred ?
            runner->geometry->getOrCreateStateSet() : camera->getOrCreateStateSet();
        ss->setTextureAttributeAndModes(u, createDefaultTexture(color));
        ss->addUniform(new osg::Uniform(buffer.data(), u));
    }

    Pipeline::Stage* Pipeline::getStage(const std::string& name)
    {
        for (size_t i = 0; i < _stages.size(); ++i)
        { if (_stages[i]->name == name) return _stages[i].get(); }
        return NULL;
    }

    void Pipeline::startStages(int w, int h)
    {
        _stageSize = osg::Vec2i(w, h);
        _stageContext = createGraphicsContext(w, h);
    }

    void Pipeline::applyStagesToView(osgViewer::View* view, unsigned int forwardMask)
    {
        for (unsigned int i = 0; i < _stages.size(); ++i)
        {
            bool useMainScene = _stages[i]->inputStage;
            if (_stages[i]->deferred || !_stages[i]->camera) continue;
            view->addSlave(_stages[i]->camera.get(), osg::Matrix(), osg::Matrix(), useMainScene);
        }

        osg::ref_ptr<osg::Camera> forwardCam = (view->getCamera() != NULL)
                                             ? new osg::Camera(*view->getCamera()) : new osg::Camera;
        forwardCam->setName("DefaultForward");
        forwardCam->setUserValue("NeedNearFarCalculation", true);
        forwardCam->setCullMask(forwardMask);
        forwardCam->setClampProjectionMatrixCallback(new MyClampProjectionCallback(_deferredCallback.get()));
        forwardCam->setComputeNearFarMode(osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        forwardCam->setPreDrawCallback(_deferredCallback.get());
        forwardCam->setViewport(0, 0, _stageSize.x(), _stageSize.y());
        forwardCam->setGraphicsContext(_stageContext.get());

        if (!_stages.empty()) forwardCam->setClearMask(0);
        view->addSlave(forwardCam.get(), osg::Matrix(), osg::Matrix(), true);
        _forwardCamera = forwardCam;
    }

    osg::GraphicsOperation* Pipeline::createRenderer(osg::Camera* camera)
    {
        MyRenderer* render = new MyRenderer(camera);
        render->useCustomSceneViews(_deferredCallback.get());
        camera->setStats(new osg::Stats("Camera"));
        return render;
    }
    
    Pipeline::Stage* Pipeline::addInputStage(const std::string& name, unsigned int cullMask,
                                             osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = false;
        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i ++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int);
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture2D> tex = createTexture(type, _stageSize[0], _stageSize[1]);
            if (i > 0) s->camera->attach(comp, tex.get());
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), false);
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        s->camera->setCullMask(cullMask);
        s->camera->setUserValue("NeedNearFarCalculation", true);
        s->camera->setClampProjectionMatrixCallback(new MyClampProjectionCallback(_deferredCallback.get()));
        s->camera->setComputeNearFarMode(osg::Camera::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
        s->inputStage = true; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addWorkStage(const std::string& name,
                                            osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = false;
        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int);
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                                              : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture2D> tex = createTexture(type, _stageSize[0], _stageSize[1]);
            if (i > 0) s->camera->attach(comp, tex.get());
            else s->camera = createRTTCamera(comp, tex.get(), _stageContext.get(), true);
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addDeferredStage(const std::string& name,
                                                osg::Shader* vs, osg::Shader* fs, int buffers, ...)
    {
        Stage* s = new Stage; s->deferred = true;
        s->runner = new osgVerse::DeferredRenderCallback::RttGeometryRunner(name);
        s->runner->setUseScreenQuad(0, NULL);  // create a quad at the beginning
        _deferredCallback->addRunner(s->runner.get());

        va_list params; va_start(params, buffers);
        for (int i = 0; i < buffers; i++)
        {
            std::string bufName = std::string(va_arg(params, const char*));
            BufferType type = (BufferType)va_arg(params, int);
            osg::Camera::BufferComponent comp = (buffers == 1) ? osg::Camera::COLOR_BUFFER
                : (osg::Camera::BufferComponent)(osg::Camera::COLOR_BUFFER0 + i);
            if (type >= DEPTH16) comp = osg::Camera::DEPTH_BUFFER;

            osg::ref_ptr<osg::Texture2D> tex = createTexture(type, _stageSize[0], _stageSize[1]);
            s->runner->attach(comp, tex.get());
            s->outputs[bufName] = tex.get();
        }
        va_end(params);

        applyDefaultStageData(*s, name, vs, fs);
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    Pipeline::Stage* Pipeline::addDisplayStage(const std::string& name,
                                               osg::Shader* vs, osg::Shader* fs, const osg::Vec4& geom)
    {
        Stage* s = new Stage; s->deferred = false;
        s->camera = createHUDCamera(_stageContext.get(), _stageSize[0], _stageSize[1],
                                    osg::Vec3(geom[0], geom[1], 0.0f), geom[2], geom[3], true);
        applyDefaultStageData(*s, name, vs, fs);
        s->inputStage = false; _stages.push_back(s);
        return s;
    }

    void Pipeline::applyDefaultStageData(Stage& s, const std::string& name, osg::Shader* vs, osg::Shader* fs)
    {
        if (vs || fs)
        {
            osg::ref_ptr<osg::Program> prog = new osg::Program;
            prog->setName(name + "_PROGRAM");
            if (vs) { vs->setName(name + "_SHADER_VS"); prog->addShader(vs); }
            if (fs) { fs->setName(name + "_SHADER_FS"); prog->addShader(fs); }

            osg::StateSet* ss = s.deferred ?
                s.runner->geometry->getOrCreateStateSet() : s.camera->getOrCreateStateSet();
            ss->setAttributeAndModes(prog.get(), osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        }
        s.name = name; s.camera->setName(name);
    }

    osg::Texture2D* Pipeline::createTexture(BufferType type, int w, int h)
    {
        osg::ref_ptr<osg::Texture2D> tex = new osg::Texture2D;
        tex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        tex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        tex->setTextureSize(w, h);
        switch (type)
        {
        case RGB_INT8:
            tex->setInternalFormat(GL_RGB);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGBA_INT8:
            tex->setInternalFormat(GL_RGBA);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_UNSIGNED_BYTE);
            break;
        case RGB_FLOAT16:
            tex->setInternalFormat(GL_RGB16F_ARB);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_FLOAT);
            break;
        case RGBA_FLOAT16:
            tex->setInternalFormat(GL_RGBA16F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT);
            break;
        case RGB_FLOAT32:
            tex->setInternalFormat(GL_RGB32F_ARB);
            tex->setSourceFormat(GL_RGB);
            tex->setSourceType(GL_FLOAT);
            break;
        case RGBA_FLOAT32:
            tex->setInternalFormat(GL_RGBA32F_ARB);
            tex->setSourceFormat(GL_RGBA);
            tex->setSourceType(GL_FLOAT);
            break;
        case R_FLOAT16:
            tex->setInternalFormat(GL_LUMINANCE16F_ARB);
            tex->setSourceFormat(GL_LUMINANCE);
            tex->setSourceType(GL_FLOAT);
            break;
        case R_FLOAT32:
            tex->setInternalFormat(GL_LUMINANCE32F_ARB);
            tex->setSourceFormat(GL_LUMINANCE);
            tex->setSourceType(GL_FLOAT);
            break;
        case DEPTH16:
            tex->setInternalFormat(GL_DEPTH_COMPONENT16);
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_FLOAT);
            break;
        case DEPTH32:
            tex->setInternalFormat(GL_DEPTH_COMPONENT32);
            tex->setSourceFormat(GL_DEPTH_COMPONENT);
            tex->setSourceType(GL_FLOAT);
            break;
        }
        return tex.release();
    }
}
