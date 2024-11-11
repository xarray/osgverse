#include <Effekseer.h>
#include <EffekseerRendererGL.h>
#include <EffekseerSoundDSound.h>
#include "ParticleEngine.h"
using namespace osgVerse;

class EffekseerData : public osg::Referenced
{
public:
    EffekseerData(int maxInstances)
    {
        EffekseerRendererGL::OpenGLDeviceType type = EffekseerRendererGL::OpenGLDeviceType::OpenGL2;
        std::string devTypeName = "OpenGL2";
#if defined(OSG_GL3_AVAILABLE)
        type = EffekseerRendererGL::OpenGLDeviceType::OpenGL3; devTypeName = "OpenGL3";
#elif defined(OSG_GLES2_AVAILABLE)
        type = EffekseerRendererGL::OpenGLDeviceType::OpenGLES2; devTypeName = "OpenGLES2";
#elif defined(OSG_GLES3_AVAILABLE)
        type = EffekseerRendererGL::OpenGLDeviceType::OpenGLES3; devTypeName = "OpenGLES3";
#endif
        efkManager = Effekseer::Manager::Create(maxInstances);

        auto graphicsDevice = EffekseerRendererGL::CreateGraphicsDevice(type);
        if (graphicsDevice == NULL)
        {
            OSG_FATAL << "[ParticleEngine] Failed to create graphics device: " << devTypeName
                      << ". Maybe Effekseer need to be recompiled" << std::endl; return;
        }

        efkRenderer = EffekseerRendererGL::Renderer::Create(graphicsDevice, maxInstances);
        if (efkRenderer == NULL)
        {
            OSG_FATAL << "[ParticleEngine] Failed to create renderer: " << devTypeName
                      << ". Maybe Effekseer need to be recompiled" << std::endl; return;
        }

        efkManager->SetSpriteRenderer(efkRenderer->CreateSpriteRenderer());
        efkManager->SetRibbonRenderer(efkRenderer->CreateRibbonRenderer());
        efkManager->SetRingRenderer(efkRenderer->CreateRingRenderer());
        efkManager->SetTrackRenderer(efkRenderer->CreateTrackRenderer());
        efkManager->SetModelRenderer(efkRenderer->CreateModelRenderer());
        efkManager->SetTextureLoader(efkRenderer->CreateTextureLoader());
        efkManager->SetModelLoader(efkRenderer->CreateModelLoader());
        efkManager->SetMaterialLoader(efkRenderer->CreateMaterialLoader());
        efkManager->SetCurveLoader(Effekseer::MakeRefPtr<Effekseer::CurveLoader>());

        efkSound = NULL;// EffekseerSound::Sound::Create();  // TODO
        if (efkSound != NULL)
        {
            efkManager->SetSoundPlayer(efkSound->CreateSoundPlayer());
            //efkManager->SetSoundLoader(efkSound->CreateSoundLoader());  // TODO
        }
    }

    bool createEffect(const std::string& name, const std::vector<unsigned char>& buffer,
                      float magnification = 1.0f)
    {
        std::map<std::string, EffectPair>::iterator itr = effects.find(name);
        if (name.empty() || buffer.empty()) return false;
        if (itr != effects.end()); { itr->second.first.Reset(); effects.erase(itr); }

        Effekseer::EffectRef effect = Effekseer::Effect::Create(
            efkManager, &buffer[0], buffer.size(), magnification);
        if (effect != NULL) effects[name] = EffectPair(effect, NULL);
        return (effect != NULL);
    }

    bool playEffect(const std::string& name, const osg::Vec3d& pos, int state, int start = 0)
    {
        std::map<std::string, EffectPair>::iterator itr = effects.find(name);
        if (itr == effects.end()) return false;

        // 0: stopped, 1: playing, 2: paused, 3: moving
        Effekseer::Vector3D efkPos(pos[0], pos[1], pos[2]);
        if (state == 1)
        {
            itr->second.second = efkManager->Play(itr->second.first, efkPos, start);
            return (itr->second.second != NULL);
        }
        else if (itr->second.second == NULL) return false;

        bool paused = efkManager->GetPaused(itr->second.second);
        if (state == 0) efkManager->StopEffect(itr->second.second);
        else if (state == 3) efkManager->AddLocation(itr->second.second, efkPos);
        else efkManager->SetPaused(itr->second.second, !paused); return true;
    }

    void update(osg::Camera* camera) const
    {
        osg::Vec3d pos = osg::Vec3d() * camera->getInverseViewMatrix();
        Effekseer::Manager::LayerParameter layerParam;
        layerParam.ViewerPosition = Effekseer::Vector3D(pos[0], pos[1], pos[2]);
        efkManager->SetLayerParameter(0, layerParam);

        Effekseer::Manager::UpdateParameter updateParam;
        efkManager->Update(updateParam);
    }

    void draw(osg::RenderInfo& renderInfo) const
    {
        osg::State* state = renderInfo.getState();
        osg::Matrixf view = state->getModelViewMatrix();
        osg::Matrixf proj = state->getProjectionMatrix();

        Effekseer::Matrix44 efkView, efkProj;
        memcpy((float*)efkView.Values, view.ptr(), sizeof(float) * 16);
        memcpy((float*)efkProj.Values, proj.ptr(), sizeof(float) * 16);
        efkRenderer->SetTime(state->getFrameStamp()->getSimulationTime());
        efkRenderer->SetProjectionMatrix(efkProj);
        efkRenderer->SetCameraMatrix(efkView);

        Effekseer::Manager::DrawParameter drawParam;
        drawParam.ZNear = 0.0f;
        drawParam.ZFar = 1.0f;
        drawParam.ViewProjectionMatrix = efkRenderer->GetCameraProjectionMatrix();

        efkRenderer->BeginRendering();
        efkManager->Draw(drawParam);
        efkRenderer->EndRendering();
    }

    void release()
    {
        efkRenderer.Reset();
        efkManager.Reset();
    }

    typedef std::pair<Effekseer::EffectRef, Effekseer::Handle> EffectPair;
    std::map<std::string, EffectPair> effects;
    Effekseer::ManagerRef efkManager;
    EffekseerRendererGL::RendererRef efkRenderer;
    EffekseerSound::SoundRef efkSound;
};

ParticleDrawable::ParticleDrawable(int maxInstances)
{
    _data = new EffekseerData(maxInstances);
    setUseDisplayList(false);
    setUseVertexBufferObjects(false);
#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
    setCullingActive(false);
#endif
}

ParticleDrawable::ParticleDrawable(const ParticleDrawable& copy, const osg::CopyOp& copyop)
    : osg::Drawable(copy, copyop), _data(copy._data)
{}

ParticleDrawable::~ParticleDrawable()
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    ed->release();
}

Effekseer::Manager* ParticleDrawable::getManager() const
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    return ed->efkManager.Get();
}

#if OSG_MIN_VERSION_REQUIRED(3, 3, 2)
osg::BoundingBox ParticleDrawable::computeBoundingBox() const
{
    osg::BoundingBox bb;
    return bb;  // TODO
}

osg::BoundingSphere ParticleDrawable::computeBound() const
{
    osg::BoundingSphere bs(computeBoundingBox());
    return bs;
}
#else
osg::BoundingBox ParticleDrawable::computeBound() const
{
    osg::BoundingBox bb;
    return bb;  // TODO
}
#endif

void ParticleDrawable::drawImplementation(osg::RenderInfo& renderInfo) const
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    ed->update(renderInfo.getCurrentCamera());
    ed->draw(renderInfo);
}
