#include <Effekseer.h>
#include <EffekseerRendererGL.h>
#include <EffekseerSoundDSound.h>

#include <osgDB/FileNameUtils>
#include "pipeline/Global.h"
#include "ParticleEngine.h"
using namespace osgVerse;

static void effekseerLog(Effekseer::LogType logType, const std::string& message)
{
    switch (logType)
    {
    case Effekseer::LogType::Info: OSG_INFO << "[ParticleDrawable] " << message << std::endl; break;
    case Effekseer::LogType::Warning: OSG_NOTICE << "[ParticleDrawable] " << message << std::endl; break;
    case Effekseer::LogType::Error: OSG_WARN << "[ParticleDrawable] " << message << std::endl; break;
    default: OSG_DEBUG << "[ParticleDrawable] " << message << std::endl; break;
    }
}

class EffekseerData : public osg::Referenced
{
public:
    EffekseerData(int maxI)
    {
        efkManager = Effekseer::Manager::Create(maxI);
        efkRenderer = NULL; maxInstances = maxI;
        Effekseer::SetLogger(effekseerLog);

        efkSound = NULL;// EffekseerSound::Sound::Create();  // TODO
        if (efkSound != NULL)
        {
            efkManager->SetSoundPlayer(efkSound->CreateSoundPlayer());
            //efkManager->SetSoundLoader(efkSound->CreateSoundLoader());  // TODO
        }
    }

    void initializeContext()
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

        for (std::map<std::string, EffectPreData>::iterator itr = preEffectMap.begin();
             itr != preEffectMap.end(); ++itr)
        {
            EffectPreData& preData = itr->second;
            createEffect(itr->first, preData.buffer, preData.materialDir, preData.magnification);
            playEffect(itr->first, preData.position, preData.state, preData.startFrame);
        }
        preEffectMap.clear();
    }

    Effekseer::Effect* createEffect(const std::string& name, const std::vector<char>& buffer,
                                    const std::wstring& mtlDir, float magnification = 1.0f)
    {
        if (!efkRenderer)
        {
            EffectPreData preData; preData.buffer = buffer;
            preData.materialDir = mtlDir;
            preData.magnification = magnification;
            preData.state = -1; preData.startFrame = 0;
            preEffectMap[name] = preData; return NULL;
        }

        std::map<std::string, EffectPair>::iterator itr = effects.find(name);
        if (name.empty() || buffer.empty()) return NULL;
        if (itr != effects.end()) { itr->second.first.Reset(); effects.erase(itr); }

        Effekseer::EffectRef effect = Effekseer::Effect::Create(
            efkManager, &buffer[0], buffer.size(), magnification, (char16_t*)mtlDir.c_str());
        if (effect != NULL) effects[name] = EffectPair(effect, NULL);
        else OSG_NOTICE << "[ParticleDrawable] Failed to create effect " << name << std::endl;
        return effect.Get();
    }

    bool playEffect(const std::string& name, const osg::Vec3d& pos, int state, int start = 0)
    {
        if (!efkRenderer)
        {
            if (preEffectMap.find(name) != preEffectMap.end())
            {
                preEffectMap[name].position = pos;
                preEffectMap[name].state = state;
                preEffectMap[name].startFrame = start;
            }
            return true;
        }

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
        if (!efkRenderer) const_cast<EffekseerData*>(this)->initializeContext();

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

    void release(bool freeManager)
    {
        efkRenderer.Reset();
        if (freeManager) efkManager.Reset();
    }

    struct EffectPreData
    {
        std::vector<char> buffer;
        std::wstring materialDir;
        osg::Vec3d position;
        float magnification;
        int state, startFrame;
    };

    typedef std::pair<Effekseer::EffectRef, Effekseer::Handle> EffectPair;
    std::map<std::string, EffectPair> effects;
    std::map<std::string, EffectPreData> preEffectMap;
    Effekseer::ManagerRef efkManager;
    EffekseerRendererGL::RendererRef efkRenderer;
    EffekseerSound::SoundRef efkSound;
    size_t maxInstances;
};

ParticleDrawable::ParticleDrawable(int maxInstances)
{
    _data = new EffekseerData(maxInstances);
    setDataVariance(osg::Object::DYNAMIC);
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
    ed->release(true);
}

void ParticleDrawable::releaseGLObjects(osg::State* state) const
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    ed->release(false);
}

Effekseer::Effect* ParticleDrawable::createEffect(const std::string& name, const std::string& fileName)
{
    std::ifstream in(fileName, std::ios::in | std::ios::binary);
    std::istreambuf_iterator<char> eos;
    std::vector<char> data(std::istreambuf_iterator<char>(in), eos);
    if (data.empty()) return NULL;

    std::string dir = osgDB::getFilePath(fileName);
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    return ed->createEffect(name, data, Utf8StringValidator::convertW(dir));
}

void ParticleDrawable::destroyEffect(const std::string& name)
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    if (ed->effects.find(name) != ed->effects.end())
        ed->effects.erase(ed->effects.find(name));
}

bool ParticleDrawable::playEffect(const std::string& name, PlayingState state)
{
    // TODO: location?
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    return ed->playEffect(name, osg::Vec3d(), (int)state);
}

ParticleDrawable::PlayingState ParticleDrawable::getEffectState(const std::string& name) const
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    if (ed->effects.find(name) == ed->effects.end()) return INVALID;

    EffekseerData::EffectPair& pair = ed->effects.find(name)->second;
    if (pair.second == NULL) return STOPPED;
    else if (ed->efkManager->GetPaused(pair.second)) return PAUSED;
    else return PLAYING;
}

Effekseer::Effect* ParticleDrawable::getEffect(const std::string& name) const
{
    EffekseerData* ed = static_cast<EffekseerData*>(_data.get());
    if (ed->effects.find(name) == ed->effects.end()) return NULL;
    return ed->effects.find(name)->second.first.Get();
    
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
