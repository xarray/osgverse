#include <osg/io_utils>
#include <osgDB/ReadFile>
#include <osgUtil/UpdateVisitor>
#include <iostream>
#include <cmath>
#include "LightModule.h"
#include "ShadowModule.h"
#include "Utilities.h"

namespace osgVerse
{
    LightModule::LightModule(const std::string& name, Pipeline* pipeline, int maxLightsInPass)
        : _pipeline(pipeline), _maxLightsInPass(maxLightsInPass)
    {
        _parameterImage = new osg::Image;
        _parameterImage->allocateImage(1024, 4, 1, GL_RGBA, GL_FLOAT);
#if defined(VERSE_EMBEDDED_GLES2)
        _parameterImage->setInternalTextureFormat(GL_RGBA);
#else
        _parameterImage->setInternalTextureFormat(GL_RGBA32F_ARB);
#endif
        // Row 3 is reserved for future use, so keep it clean from the very beginning
        memset(_parameterImage->data(), 0, _parameterImage->getTotalSizeInBytes());

        _parameterTex = new osg::Texture2D;
        _parameterTex->setImage(_parameterImage.get());
        _parameterTex->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
        _parameterTex->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
        _parameterTex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        _parameterTex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        _parameterTex->setBorderColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));

        _lightNumber = new osg::Uniform("LightNumber", osg::Vec2(0.0f, (float)maxLightsInPass));
        if (pipeline) pipeline->addModule(name, this);
    }

    LightModule::~LightModule()
    {
        if (_pipeline.valid()) _pipeline->removeModule(this);
    }

    void LightModule::operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osgUtil::UpdateVisitor* uv = static_cast<osgUtil::UpdateVisitor*>(nv);
        if (!uv) { traverse(node, nv); return; }

        // Update shadow module if main-light exists
        const float dirLength = 1000.0f;
        if (_pipeline.valid() && _mainLight.valid())
        {
            ShadowModule* shadow = static_cast<ShadowModule*>(_pipeline->getModule(_shadowModuleName));
            if (shadow != NULL)
            {
                // FIXME: what if main-light is not directional?
                osg::Matrix worldM = _mainLight->getWorldMatrices()[0];
                osg::Vec3d pos0 = _mainLight->getPosition() * worldM;
                osg::Vec3d pos1 = (_mainLight->getPosition() +
                                   _mainLight->getDirection() * dirLength) * worldM;
                shadow->setLightState(pos0, pos1 - pos0);
            }
        }

        // Prune global light manager
        if (uv->getFrameStamp() && !(uv->getFrameStamp()->getFrameNumber() % 10))
            LightGlobalManager::instance()->prune(uv->getFrameStamp());
        if (!LightGlobalManager::instance()->checkDirty())
        { traverse(node, nv); return; }

        // Get and sort lights by its importance (e.g., last frame number, distance to eye)
        std::vector<LightGlobalManager::LightData> resultLights;
        size_t numData = LightGlobalManager::instance()->getSortedResult(resultLights);
        if (numData > 1024) numData = 1024;

        // Save all lights to a parameter texture to use in deferred shader.
        // The layout must match getLightAttributes() in std_pbr_lighting.frag.glsl and
        // std_forward_render.frag.glsl:
        //   row0: light color (vec3) + type (float)
        //   row1: eye-space position (vec3) + attenuation range (float)
        //   row2: eye-space direction (vec3) + spot cutoff as cosine (float)
        osg::Vec4f* paramPtr = (osg::Vec4f*)_parameterImage->data();
        for (size_t i = 0; i < numData; ++i)
        {
            LightGlobalManager::LightData& ld = resultLights[i];
            if (!ld.light) continue; bool unlimited = false;
            LightDrawable::Type t = ld.light->getType(unlimited);
            const osg::Vec3& color = ld.light->getColor();
            osg::Vec3 pos0 = ld.light->getPosition() * ld.matrix;
            osg::Vec3 pos1 = (ld.light->getPosition() +
                              ld.light->getDirection() * dirLength) * ld.matrix;
            osg::Vec3 dir = pos1 - pos0; dir.normalize();

            // The shader compares the spot cutoff with dot(spot_dir, light_dir), so a cosine
            // is expected here; LightDrawable::getSpotCutoff() stores the half-angle in radians
            float spotCosine = cosf(ld.light->getSpotCutoff());
            *(paramPtr + 1024 * 0 + i)/*light color, type*/ =
                osg::Vec4f(color[0], color[1], color[2], (float)t);
            *(paramPtr + 1024 * 1 + i)/*eye-space position, range*/ =
                osg::Vec4f(pos0[0], pos0[1], pos0[2], ld.light->getRange());
            *(paramPtr + 1024 * 2 + i)/*eye-space direction, spot cutoff*/ =
                osg::Vec4f(dir[0], dir[1], dir[2], spotCosine);
        }
        _lightNumber->set(osg::Vec2((float)numData, (float)_maxLightsInPass));
        _parameterImage->dirty();
        traverse(node, nv);
    }

    int LightModule::applyTextureAndUniforms(Pipeline::Stage* stage,
                                             const std::string& prefix, int startU)
    {
        stage->applyTexture(_parameterTex.get(), prefix, startU);
        stage->applyUniform(getLightNumber());
        return startU + 1;
    }

    LightGlobalManager* LightGlobalManager::instance()
    {
        static osg::ref_ptr<LightGlobalManager> s_instance = new LightGlobalManager;
        return s_instance.get();
    }

    LightGlobalManager::LightGlobalManager()
    { _callback = new LightCullCallback; _dirty = false; }

    size_t LightGlobalManager::getSortedResult(std::vector<LightData>& result)
    {
        for (std::map<LightDrawable*, LightData>::iterator itr = _lights.begin();
             itr != _lights.end(); ++itr)
        { result.push_back(itr->second); }

        std::sort(result.begin(), result.end(),
                  [](const LightData& l, const LightData& r) {
            // TODO: more sort comparers, like distance-to-eye?
            return l.frameNo > r.frameNo;
        });
        return result.size();
    }

    void LightGlobalManager::remove(LightDrawable* light)
    {
        if (_lights.find(light) != _lights.end())
        { _lights.erase(_lights.find(light)); _dirty = true; }
    }

    void LightGlobalManager::prune(const osg::FrameStamp* fs, int outdatedFrames)
    {
        unsigned int frameNo = fs->getFrameNumber();
        for (std::map<LightDrawable*, LightData>::iterator itr = _lights.begin();
             itr != _lights.end();)
        {
            if ((itr->second.frameNo + outdatedFrames) >= frameNo) itr++;
            else { itr = _lights.erase(itr); _dirty = true; }
        }
    }
}
