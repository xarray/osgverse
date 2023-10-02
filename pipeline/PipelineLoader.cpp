#include "Pipeline.h"
#include "ShadowModule.h"
#include "LightModule.h"
#include "Utilities.h"
#include <picojson.h>

#include <osg/GLExtensions>
#include <osg/DisplaySettings>
#include <osgDB/ReadFile>
#include <osgDB/FileNameUtils>
#include <osgViewer/Viewer>
extern void obtainScreenResolution(unsigned int& w, unsigned int& h);

namespace osgVerseUtils
{
    static std::string loadInclusion(picojson::value& root)
    {
        std::string sourceData;
        if (root.contains("source"))
        {
            picojson::value& source = root.get("source");
            if (source.is<picojson::array>())
            {
                picojson::array& sourceList = source.get<picojson::array>();
                for (size_t i = 0; i < sourceList.size(); ++i)
                    sourceData += sourceList[i].to_str() + "\n";
            }
            else
                sourceData = source.to_str();
        }
        else if (root.contains("uri"))
        {
            std::ifstream in(root.get("uri").to_str());
            std::istreambuf_iterator<char> eos;
            sourceData = std::string(std::istreambuf_iterator<char>(in), eos);
        }
        return sourceData;
    }

    static osg::Shader* loadShader(picojson::value& root,
                                   std::map<std::string, std::string>& inclusions)
    {
        std::string inclusionData;
        if (root.contains("include"))
        {
            picojson::value& incNode = root.get("include");
            if (incNode.is<picojson::array>())
            {
                picojson::array& incList = incNode.get<picojson::array>();
                for (size_t i = 0; i < incList.size(); ++i)
                {
                    std::string incName = incList[i].to_str();
                    if (inclusions.find(incName) != inclusions.end())
                        inclusionData += inclusions[incName] + "\n";
                }
            }
        }

        osg::Shader* shader = new osg::Shader;
        shader->setName(root.get("name").to_str());
        if (root.contains("source"))
        {
            picojson::value& source = root.get("source");
            if (source.is<picojson::array>())
            {
                std::string sourceData;
                picojson::array& sourceList = source.get<picojson::array>();
                for (size_t i = 0; i < sourceList.size(); ++i)
                    sourceData += sourceList[i].to_str() + "\n";
                shader->setShaderSource(inclusionData + sourceData);
            }
            else
                shader->setShaderSource(inclusionData + source.to_str());
        }
        else if (root.contains("binary"))
        {
            osg::ShaderBinary* sb = osg::ShaderBinary::readShaderBinaryFile(root.get("binary").to_str());
            if (sb) shader->setShaderBinary(sb);
        }
        else if (root.contains("uri"))
            shader->loadShaderSourceFromFile(root.get("uri").to_str());

        std::string type = root.get("shader_type").to_str();
        if (type.find("vertex") != std::string::npos) shader->setType(osg::Shader::VERTEX);
        else if (type.find("geom") != std::string::npos) shader->setType(osg::Shader::GEOMETRY);
        else if (type.find("control") != std::string::npos) shader->setType(osg::Shader::TESSCONTROL);
        else if (type.find("eval") != std::string::npos) shader->setType(osg::Shader::TESSEVALUATION);
        else shader->setType(osg::Shader::FRAGMENT);
        return shader;
    }

    static osg::Texture* loadTexture(picojson::value& root, bool isIblData,
                                     std::map<std::string, osg::ref_ptr<osg::Texture>>& textures)
    {
        if (!isIblData)
        {
            // "uri", "texture_type" / "function", "arguments"
        }
        else
        {
            // "uri": "...", "index": 0
        }
        return NULL;
    }

    static void setUniformValue(osg::Uniform* u, int idx, const std::string& v)
    {
        std::vector<int> iv; std::vector<float> fv;
        osgDB::StringList sList; osgDB::split(v, sList);
        for (size_t i = 0; i < sList.size(); ++i)
        { iv.push_back(std::stoi(sList[i])); fv.push_back(std::stof(sList[i])); }

        size_t l = iv.size(); if (iv.empty() || fv.empty()) return;
        switch (u->getType())
        {
        case osg::Uniform::BOOL:
            if (idx < 0) u->set(iv[0] > 0); else u->setElement(idx, iv[0] > 0); break;
        case osg::Uniform::BOOL_VEC2:
            if (l < 2) break; else if (idx < 0) u->set(iv[0] > 0, iv[1] > 0);
            else u->setElement(idx, iv[0] > 0, iv[1] > 0); break;
        case osg::Uniform::BOOL_VEC3:
            if (l < 3) break; else if (idx < 0) u->set(iv[0] > 0, iv[1] > 0, iv[2] > 0);
            else u->setElement(idx, iv[0] > 0, iv[1] > 0, iv[2] > 0); break;
        case osg::Uniform::BOOL_VEC4:
            if (l < 4) break; else if (idx < 0) u->set(iv[0] > 0, iv[1] > 0, iv[2] > 0, iv[3] > 0);
            else u->setElement(idx, iv[0] > 0, iv[1] > 0, iv[2] > 0, iv[3] > 0); break;
        case osg::Uniform::INT:
            if (idx < 0) u->set(iv[0]); else u->setElement(idx, iv[0]); break;
        case osg::Uniform::INT_VEC2:
            if (l < 2) break; else if (idx < 0) u->set(iv[0], iv[1]);
            else u->setElement(idx, iv[0], iv[1]); break;
        case osg::Uniform::INT_VEC3:
            if (l < 3) break; else if (idx < 0) u->set(iv[0], iv[1], iv[2]);
            else u->setElement(idx, iv[0], iv[1], iv[2]); break;
        case osg::Uniform::INT_VEC4:
            if (l < 4) break; else if (idx < 0) u->set(iv[0], iv[1], iv[2], iv[3]);
            else u->setElement(idx, iv[0], iv[1], iv[2], iv[3]); break;
        case osg::Uniform::FLOAT:
            if (idx < 0) u->set(fv[0]); else u->setElement(idx, fv[0]); break;
        case osg::Uniform::FLOAT_VEC2:
            if (l < 2) break; else if (idx < 0) u->set(osg::Vec2(fv[0], fv[1]));
            else u->setElement(idx, osg::Vec2(fv[0], fv[1])); break;
        case osg::Uniform::FLOAT_VEC3:
            if (l < 3) break; else if (idx < 0) u->set(osg::Vec3(fv[0], fv[1], fv[2]));
            else u->setElement(idx, osg::Vec3(fv[0], fv[1], fv[2])); break;
        case osg::Uniform::FLOAT_VEC4:
            if (l < 4) break; else if (idx < 0) u->set(osg::Vec4(fv[0], fv[1], fv[2], fv[3]));
            else u->setElement(idx, osg::Vec4(fv[0], fv[1], fv[2], fv[3])); break;
        case osg::Uniform::FLOAT_MAT2:
            if (l < 4) break; else if (idx < 0) u->set(osg::Matrix2(fv[0], fv[1], fv[2], fv[3]));
            else u->setElement(idx, osg::Matrix2(fv[0], fv[1], fv[2], fv[3])); break;
        case osg::Uniform::FLOAT_MAT3:
            if (l < 9) break; else if (idx < 0) u->set(osg::Matrix3(fv[0], fv[1], fv[2], fv[3], fv[4], fv[5], fv[6], fv[7], fv[8]));
            else u->setElement(idx, osg::Matrix3(fv[0], fv[1], fv[2], fv[3], fv[4], fv[5], fv[6], fv[7], fv[8])); break;
        case osg::Uniform::FLOAT_MAT4:
            if (l < 16) break; else if (idx < 0) u->set(osg::Matrixf(&fv[0]));
            else u->setElement(idx, osg::Matrixf(&fv[0])); break;
        default:
            if (idx < 0) u->set((unsigned int)iv[0]); else u->setElement(idx, (unsigned int)iv[0]); break;
        }
    }

    static osg::Uniform* loadUniform(picojson::value& root)
    {
        std::string name = root.contains("uniform_name")
                         ? root.get("uniform_name").to_str() : root.get("name").to_str();
        std::string type = root.get("uniform_type").to_str();

        osg::Uniform::Type t = osg::Uniform::FLOAT;
        if (type.find("bool") != std::string::npos) t = osg::Uniform::BOOL;
        else if (type.find("bvec2") != std::string::npos) t = osg::Uniform::BOOL_VEC2;
        else if (type.find("bvec3") != std::string::npos) t = osg::Uniform::BOOL_VEC3;
        else if (type.find("bvec4") != std::string::npos) t = osg::Uniform::BOOL_VEC4;
        else if (type.find("uint") != std::string::npos) t = osg::Uniform::UNSIGNED_INT;
        else if (type.find("int") != std::string::npos) t = osg::Uniform::INT;
        else if (type.find("ivec2") != std::string::npos) t = osg::Uniform::INT_VEC2;
        else if (type.find("ivec3") != std::string::npos) t = osg::Uniform::INT_VEC3;
        else if (type.find("ivec4") != std::string::npos) t = osg::Uniform::INT_VEC4;
        else if (type.find("vec2") != std::string::npos) t = osg::Uniform::FLOAT_VEC2;
        else if (type.find("vec3") != std::string::npos) t = osg::Uniform::FLOAT_VEC3;
        else if (type.find("vec4") != std::string::npos) t = osg::Uniform::FLOAT_VEC4;
        else if (type.find("mat2") != std::string::npos) t = osg::Uniform::FLOAT_MAT2;
        else if (type.find("mat3") != std::string::npos) t = osg::Uniform::FLOAT_MAT3;
        else if (type.find("mat4") != std::string::npos) t = osg::Uniform::FLOAT_MAT4;
        else if (type.find("1d") != std::string::npos) t = osg::Uniform::SAMPLER_1D;
        else if (type.find("2d") != std::string::npos) t = osg::Uniform::SAMPLER_2D;
        else if (type.find("2d_array") != std::string::npos) t = osg::Uniform::SAMPLER_2D_ARRAY;
        else if (type.find("3d") != std::string::npos) t = osg::Uniform::SAMPLER_3D;
        else if (type.find("cube") != std::string::npos) t = osg::Uniform::SAMPLER_CUBE;

        osg::Uniform* uniform = new osg::Uniform(t, name);
        if (root.contains("value"))
        {
            picojson::value& value = root.get("value");
            if (value.is<picojson::array>())
            {
                picojson::array& valueList = value.get<picojson::array>();
                uniform->setNumElements(valueList.size());
                for (size_t i = 0; i < valueList.size(); ++i)
                    setUniformValue(uniform, i, valueList[i].to_str());
            }
            else
                setUniformValue(uniform, -1, value.to_str());
        }
        return uniform;
    }
}

namespace osgVerse
{
    /* {
    *    "pipeline": [
    *      { "stages": [
    *        { "name": "..", "type": "input/deferred/work/display/shadow_module/light_module",
    *          <"scale": "1">, <"runOnce": "false">,
    *          "inputs": [ {"name": "..", <"type": "..">, <"uri": "..">, <"unit": "..">} ],
    *          "outputs": [ {"name": "..", "type": ".."} ],
    *          "shaders": [ {"name": "..", <"type": "..">, <"uri": "..">} ],
    *          "uniforms": [ {"name": "..", <"type": "..">, <"value": "..">} ]
    *        }, { ... } ]
    *      }, { "stage": [...] }, ...
    *    ],
    *    "shared": [{"type": "shader/texture/uniform/ibl_data/inclusion", "name": ".."}, {}, {}]
    *    "settings": {"width": 1920, "height": 1080, "masks": {..}},
    *  }
    */
    bool Pipeline::load(std::istream& in, osgViewer::View* view, osg::Camera* mainCam)
    {
        picojson::value root;
        std::string err = picojson::parse(root, in);
        if (err.empty())
        {
            picojson::value& pipeline = root.get("pipeline");
            picojson::value& shared = root.get("shared");
            picojson::value& props = root.get("settings");

            if (!mainCam) mainCam = view->getCamera();
            if (!view)
            {
                OSG_WARN << "[Pipeline] No view provided." << std::endl;
                return false;
            }
#if OSG_VERSION_GREATER_THAN(3, 2, 0)
            else if (!view->getLastAppliedViewConfig() && !mainCam->getGraphicsContext())
#else
            else if (!mainCam->getGraphicsContext())
#endif
            {
                OSG_NOTICE << "[Pipeline] No view config applied. The pipeline will be constructed "
                           << "with provided parameters. Please DO NOT apply any view config like "
                           << "setUpViewInWindow() or setUpViewAcrossAllScreens() AFTER you called "
                           << "setupStandardPipeline(). It may cause problems!!" << std::endl;
            }

            unsigned int width = 0, height = 0; obtainScreenResolution(width, height);
            if (props.contains("width")) width = props.get("width").get<double>();
            if (props.contains("height")) height = props.get("height").get<double>();
            if (!width) width = 1920; if (!height) height = 1080;

            bool supportDrawBuffersMRT = true;
            if (!_glVersionData) _glVersionData = queryOpenGLVersion(this);
            if (_glVersionData.valid())
            {
                if (!_glVersionData->glslSupported || !_glVersionData->fboSupported)
                {
                    OSG_FATAL << "[Pipeline] Necessary OpenGL features missing. The pipeline "
                              << "can not work on your machine at present." << std::endl;
                    return false;
                }
                supportDrawBuffersMRT &= _glVersionData->drawBuffersSupported;
            }

            unsigned int deferredMask = DEFERRED_SCENE_MASK,
                         forwardMask = FORWARD_SCENE_MASK,
                         fixedShadingMask = FIXED_SHADING_MASK,
                         shadowCastMask = SHADOW_CASTER_MASK;
            if (props.contains("masks"))
            {
                picojson::value& masks = props.get("masks");
                if (masks.contains("deferred"))
                    deferredMask = std::stoi(masks.get("deferred").to_str(), 0, 16);
                if (masks.contains("forward"))
                    forwardMask = std::stoi(masks.get("forward").to_str(), 0, 16);
                if (masks.contains("forward_shading"))
                    fixedShadingMask = std::stoi(masks.get("forward_shading").to_str(), 0, 16);
                if (masks.contains("shadow_caster"))
                    shadowCastMask = std::stoi(masks.get("shadow_caster").to_str(), 0, 16);
            }

            unsigned int shadowNumber = 0, shadowRes = 1024;
            if (props.contains("shadow_number"))
                shadowNumber = props.get("shadow_number").get<double>();
            if (props.contains("shadow_resolution"))
                shadowRes = props.get("shadow_resolution").get<double>();

            std::map<std::string, osg::ref_ptr<osg::Shader>> sharedShaders;
            std::map<std::string, osg::ref_ptr<osg::Texture>> sharedTextures;
            std::map<std::string, osg::ref_ptr<osg::Uniform>> sharedUniforms;
            std::map<std::string, std::string> sharedInclusions;
            if (shared.is<picojson::array>())
            {
                picojson::array& sharedArray = shared.get<picojson::array>();
                for (size_t i = 0; i < sharedArray.size(); ++i)
                {
                    picojson::value& element = sharedArray[i];
                    if (!element.contains("name") || !element.contains("type"))
                    {
                        OSG_NOTICE << "[Pipeline] Unknown element in 'shared'"
                                   << std::endl; continue;
                    }

                    std::string name = element.get("name").to_str();
                    std::string type = element.get("type").to_str();
                    if (type.find("shader") != std::string::npos)  // shader
                        sharedShaders[name] = osgVerseUtils::loadShader(element, sharedInclusions);
                    else if (type.find("texture") != std::string::npos)
                        sharedTextures[name] = osgVerseUtils::loadTexture(element, false, sharedTextures);
                    else if (type.find("ibl_data") != std::string::npos)
                        sharedTextures[name] = osgVerseUtils::loadTexture(element, true, sharedTextures);
                    else if (type.find("uniform") != std::string::npos)
                        sharedUniforms[name] = osgVerseUtils::loadUniform(element);
                    else if (type.find("inclusion") != std::string::npos)
                        sharedInclusions[name] = osgVerseUtils::loadInclusion(element);
                    else
                        OSG_NOTICE << "[Pipeline] Unknown element " << type
                                   << " in 'shared'" << std::endl;
                }
            }

            if (pipeline.is<picojson::array>())
            {
                std::vector<Stage*> inputStages;
                startStages(width, height, mainCam->getGraphicsContext());

                picojson::array& ppArray = pipeline.get<picojson::array>();
                for (size_t i = 0; i < ppArray.size(); ++i)
                {
                    picojson::value& stageGroup = ppArray[i];
                    std::string sgName = stageGroup.get("name").to_str();
                    if (!stageGroup.contains("stages")) continue;

                    picojson::value& stages = stageGroup.get("stages");
                    if (stages.is<picojson::array>())
                    {
                        picojson::array& sgArray = stages.get<picojson::array>();
                        for (size_t s = 0; s < sgArray.size(); ++s)
                        {
                            picojson::value& stage = sgArray[s];
                            if (!stage.contains("name") || !stage.contains("type"))
                            {
                                OSG_NOTICE << "[Pipeline] Unknown stage data: "
                                           << stage.to_str() << std::endl; continue;
                            }

                            std::string name = stage.get("name").to_str();
                            std::string type = stage.get("type").to_str();

                            picojson::value& inputs = stage.get("inputs");
                            picojson::value& outputs = stage.get("outputs");
                            picojson::value& uniforms = stage.get("uniforms");
                            picojson::value& shaders = stage.get("shaders");
                            // TODO
                        }
                    }
                }

                applyStagesToView(view, mainCam, forwardMask, fixedShadingMask);
                for (size_t i = 0; i < inputStages.size(); ++i)
                    requireDepthBlit(inputStages[i], true);
            }
        }
        else
            OSG_WARN << "[Pipeline] Unable to load pipeline preset: " << err << std::endl;
        return false;
    }
}
