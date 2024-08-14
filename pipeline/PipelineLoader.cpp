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
                                     std::map<std::string, osg::ref_ptr<osg::Image>>& iblData)
    {
        std::string uri, options, name = root.get("name").to_str();
        if (root.contains("uri")) uri = root.get("uri").to_str();
        if (root.contains("options")) options = root.get("options").to_str();

        osg::Image* image = NULL;
        if (!isIblData)
        {
            if (uri.empty() && root.contains("function"))
            {
                std::string funcName = root.get("function").to_str();
                std::string args = root.contains("arguments")
                                 ? root.get("arguments").to_str() : "";
                std::stringstream ss; ss << args;
                if (funcName == "poisson")
                {
                    int cols = 0, rows = 0; ss >> cols >> rows;
                    return osgVerse::generatePoissonDiscDistribution(cols, rows);
                }
                else if (funcName == "const")
                {
                    osg::Vec4 c; ss >> c[0] >> c[1] >> c[2] >> c[3];
                    return osgVerse::createDefaultTexture(c);
                }
                else
                    OSG_WARN << "[Pipeline] Unknown texture function: " << funcName
                             << " for texture " << name << " while loading pipeline" << std::endl;
            }
            else
                image = osgDB::readImageFile(uri, new osgDB::Options(options));
        }
        else
        {
            osg::ImageSequence* seq = NULL;
            if (iblData.find(uri) != iblData.end())
                seq = dynamic_cast<osg::ImageSequence*>(iblData[uri].get());
            else if (!uri.empty())
            {
                image = osgDB::readImageFile(uri, new osgDB::Options(options));
                if (image != NULL) seq = dynamic_cast<osg::ImageSequence*>(image);
            }

            if (seq != NULL)
            {
                int index = root.contains("index") ? root.get("index").get<double>() : 0;
                image = seq->getImage(index); iblData[uri] = seq;
            }
        }

        if (image == NULL)
        {
            OSG_WARN << "[Pipeline] No valid image loaded for: " << name
                     << " while loading pipeline" << std::endl;
            return NULL;
        }

        osg::Texture::WrapMode wrapMode = osg::Texture::REPEAT;
        if (root.contains("wrap"))
        {
            std::string wrap = root.get("wrap").to_str();
            if (wrap == "mirror") wrapMode = osg::Texture::MIRROR;
            else if (wrap == "clamp") wrapMode = osg::Texture::CLAMP;
        }

        osg::Texture* texture = osgVerse::createTexture2D(image, wrapMode);
        if (root.contains("filter"))
        {
            std::string filter = root.get("filter").to_str();
            if (filter == "linear")
            {
                texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            }
            else if (filter == "nearest")
            {
                texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
                texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);
            }
            else
            {
                texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
                texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
            }
        }
        texture->setName(name);
        return texture;
    }

    static void setUniformValue(osg::Uniform* u, int idx, const std::string& v)
    {
        std::vector<int> iv; std::vector<float> fv;
        osgDB::StringList sList; osgDB::split(v, sList);
        for (size_t i = 0; i < sList.size(); ++i)
        { iv.push_back(atoi(sList[i].c_str())); fv.push_back(atof(sList[i].c_str())); }

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
    bool Pipeline::load(std::istream& in, osgViewer::View* view, osg::Camera* mainCam, bool asEmbedded)
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
#if OSG_VERSION_GREATER_THAN(3, 2, 3)
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
            if (!_glVersionData)
                _glVersionData = queryOpenGLVersion(this, asEmbedded, mainCam->getGraphicsContext());
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
                         shadowCastMask = SHADOW_CASTER_MASK;
            if (props.contains("masks"))
            {
                picojson::value& masks = props.get("masks");
                try
                {
                    if (masks.contains("deferred"))
                        deferredMask = std::stoul(masks.get("deferred").to_str(), 0, 16);
                    if (masks.contains("forward"))
                        forwardMask = std::stoul(masks.get("forward").to_str(), 0, 16);
                    if (masks.contains("shadow_caster"))
                        shadowCastMask = std::stoul(masks.get("shadow_caster").to_str(), 0, 16);
                }
                catch (std::exception& e)
                { OSG_WARN << "[Pipeline] " << e.what() << " while reading masks" << std::endl; }
            }

            unsigned int shadowNumber = 0, shadowRes = 1024;
            if (props.contains("shadow_number"))
                shadowNumber = props.get("shadow_number").get<double>();
            if (props.contains("shadow_resolution"))
                shadowRes = props.get("shadow_resolution").get<double>();

            std::map<std::string, osg::ref_ptr<osg::Shader>> sharedShaders;
            std::map<std::string, osg::ref_ptr<osg::Texture>> sharedTextures;
            std::map<std::string, osg::ref_ptr<osg::Uniform>> sharedUniforms;
            std::map<std::string, osg::ref_ptr<osg::Image>> sharedIblData;
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
                        sharedTextures[name] = osgVerseUtils::loadTexture(element, false, sharedIblData);
                    else if (type.find("ibl_data") != std::string::npos)
                        sharedTextures[name] = osgVerseUtils::loadTexture(element, true, sharedIblData);
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
                        for (size_t j = 0; j < sgArray.size(); ++j)
                        {
                            picojson::value& stage = sgArray[j];
                            if (!stage.contains("name") || !stage.contains("type"))
                            {
                                OSG_NOTICE << "[Pipeline] Unknown stage data: "
                                           << stage.to_str() << std::endl; continue;
                            }

                            std::string name = stage.get("name").to_str();
                            std::string type = stage.get("type").to_str();
                            picojson::array emptyVar;
                            picojson::array& inputs = stage.get("inputs").is<picojson::array>()
                                ? stage.get("inputs").get<picojson::array>() : emptyVar;
                            picojson::array& outputs = stage.get("outputs").is<picojson::array>()
                                ? stage.get("outputs").get<picojson::array>() : emptyVar;
                            picojson::array& uniforms = stage.get("uniforms").is<picojson::array>()
                                ? stage.get("uniforms").get<picojson::array>() : emptyVar;
                            picojson::array& shaders = stage.get("shaders").is<picojson::array>()
                                ? stage.get("shaders").get<picojson::array>() : emptyVar;

                            std::vector<osg::ref_ptr<osg::Shader>> inShaders;
                            size_t vIdx = 0, fIdx = 0;
                            for (size_t n = 0; n < shaders.size(); ++n)
                            {
                                std::string shName = shaders[n].get("name").to_str();
                                osg::ref_ptr<osg::Shader> s = NULL;
                                if (sharedShaders.find(shName) == sharedShaders.end()) s = osgVerseUtils::loadShader(shaders[n], sharedInclusions);
                                else s = sharedShaders[shName];
                                if (!s) { OSG_WARN << "[Pipeline] No such shader " << shName << "\n"; continue; }
                                if (s->getType() == osg::Shader::VERTEX) vIdx = inShaders.size();
                                if (s->getType() == osg::Shader::FRAGMENT) fIdx = inShaders.size();
                                inShaders.push_back(s);
                            }

                            Stage* s = NULL;
                            if (type.find("module") != std::string::npos)
                            {
                                if (type.find("shadow") != std::string::npos &&
                                    !inputStages.empty() && !inShaders.empty())
                                {
                                    osg::ref_ptr<osgVerse::ShadowModule> shadowModule =
                                        new osgVerse::ShadowModule(name, this, false);
                                    shadowModule->createStages(shadowRes, shadowNumber,
                                        inShaders[vIdx], inShaders[fIdx], shadowCastMask);

                                    // Update shadow matrices at the end of g-buffer (when near/far planes are sync-ed)
                                    osg::ref_ptr<osgVerse::ShadowDrawCallback> shadowCallback =
                                        new osgVerse::ShadowDrawCallback(shadowModule.get());
                                    shadowCallback->setup(inputStages.back()->camera.get(), FINAL_DRAW);
                                    mainCam->addUpdateCallback(shadowModule.get());
                                }
                                else if (type.find("light") != std::string::npos)
                                {
                                    // Light module only needs to be added to main camera
                                    osg::ref_ptr<osgVerse::LightModule> lightModule =
                                        new osgVerse::LightModule(name, this);
                                    mainCam->addUpdateCallback(lightModule.get());
                                }
                                else
                                    OSG_WARN << "[Pipeline] Invalid module data: " << name << std::endl;
                            }
                            else if (type != "display" && !inShaders.empty())
                            {
                                // Find all outputs
#define CHK_OUTPUT(fmt) \
    else if (oFmt == #fmt) inOutputs.push_back(std::pair<std::string, int>(oName, fmt))
                                std::vector<std::pair<std::string, int>> inOutputs;
                                for (size_t n = 0; n < outputs.size(); ++n)
                                {
                                    std::string oName = outputs[n].get("name").to_str();
                                    std::string oFmt = outputs[n].contains("format")
                                                     ? outputs[n].get("format").to_str() : "";
                                    if (oFmt.empty())
                                        inOutputs.push_back(std::pair<std::string, int>(oName, RGB_INT8));
                                    CHK_OUTPUT(RGB_INT8); CHK_OUTPUT(RGB_INT5); CHK_OUTPUT(RGB_INT10);
                                    CHK_OUTPUT(RGB_FLOAT16); CHK_OUTPUT(RGB_FLOAT32); CHK_OUTPUT(SRGB_INT8);
                                    CHK_OUTPUT(RGBA_INT8); CHK_OUTPUT(RGBA_INT5_1); CHK_OUTPUT(RGBA_INT10_2);
                                    CHK_OUTPUT(RGBA_FLOAT16); CHK_OUTPUT(RGBA_FLOAT32); CHK_OUTPUT(SRGBA_INT8);
                                    CHK_OUTPUT(R_INT8); CHK_OUTPUT(R_FLOAT16); CHK_OUTPUT(R_FLOAT32);
                                    CHK_OUTPUT(RG_INT8); CHK_OUTPUT(RG_FLOAT16); CHK_OUTPUT(RG_FLOAT32);
                                    CHK_OUTPUT(DEPTH16); CHK_OUTPUT(DEPTH24_STENCIL8); CHK_OUTPUT(DEPTH32);
                                    else OSG_WARN << "[Pipeline] Invalid output: " << oName << std::endl;
                                }

                                // Create stage from outputs and save inputStages
                                float scale = stage.contains("scale")
                                            ? stage.get("scale").get<double>() : 1.0f;
                                bool once = stage.contains("once")
                                          ? stage.get("once").get<bool>() : false;
                                if (!inOutputs.empty())
                                {
#define STAGE_ARG1(a) inShaders[vIdx], inShaders[fIdx], 1, a[0].first.c_str(), a[0].second
#define STAGE_ARG2(a) inShaders[vIdx], inShaders[fIdx], 2, a[0].first.c_str(), a[0].second, \
                      a[1].first.c_str(), a[1].second
#define STAGE_ARG3(a) inShaders[vIdx], inShaders[fIdx], 3, a[0].first.c_str(), a[0].second, \
                      a[1].first.c_str(), a[1].second, a[2].first.c_str(), a[2].second
#define STAGE_ARG4(a) inShaders[vIdx], inShaders[fIdx], 4, a[0].first.c_str(), a[0].second, \
                      a[1].first.c_str(), a[1].second, a[2].first.c_str(), a[2].second, a[3].first.c_str(), a[3].second
#define STAGE_ARG5(a) inShaders[vIdx], inShaders[fIdx], 5, a[0].first.c_str(), a[0].second, \
                      a[1].first.c_str(), a[1].second, a[2].first.c_str(), a[2].second, \
                      a[3].first.c_str(), a[3].second, a[4].first.c_str(), a[4].second
#define STAGE_ARG6(a) inShaders[vIdx], inShaders[fIdx], 6, a[0].first.c_str(), a[0].second, \
                      a[1].first.c_str(), a[1].second, a[2].first.c_str(), a[2].second, \
                      a[3].first.c_str(), a[3].second, a[4].first.c_str(), a[4].second, a[5].first.c_str(), a[5].second
                                    if (type == "input")
                                    {
                                        switch (inOutputs.size())
                                        {
                                        case 1: s = addInputStage(name, deferredMask, 0, STAGE_ARG1(inOutputs)); break;
                                        case 2: s = addInputStage(name, deferredMask, 0, STAGE_ARG2(inOutputs)); break;
                                        case 3: s = addInputStage(name, deferredMask, 0, STAGE_ARG3(inOutputs)); break;
                                        case 4: s = addInputStage(name, deferredMask, 0, STAGE_ARG4(inOutputs)); break;
                                        case 5: s = addInputStage(name, deferredMask, 0, STAGE_ARG5(inOutputs)); break;
                                        default: s = addInputStage(name, deferredMask, 0, STAGE_ARG6(inOutputs)); break;
                                        }
                                        inputStages.push_back(s);
                                    }
                                    else if (type == "work")
                                    {
                                        switch (inOutputs.size())
                                        {
                                        case 1: s = addWorkStage(name, scale, STAGE_ARG1(inOutputs)); break;
                                        case 2: s = addWorkStage(name, scale, STAGE_ARG2(inOutputs)); break;
                                        case 3: s = addWorkStage(name, scale, STAGE_ARG3(inOutputs)); break;
                                        case 4: s = addWorkStage(name, scale, STAGE_ARG4(inOutputs)); break;
                                        case 5: s = addWorkStage(name, scale, STAGE_ARG5(inOutputs)); break;
                                        default: s = addWorkStage(name, scale, STAGE_ARG6(inOutputs)); break;
                                        }
                                    }
                                    else
                                    {
                                        switch (inOutputs.size())
                                        {
                                        case 1: s = addDeferredStage(name, scale, once, STAGE_ARG1(inOutputs)); break;
                                        case 2: s = addDeferredStage(name, scale, once, STAGE_ARG2(inOutputs)); break;
                                        case 3: s = addDeferredStage(name, scale, once, STAGE_ARG3(inOutputs)); break;
                                        case 4: s = addDeferredStage(name, scale, once, STAGE_ARG4(inOutputs)); break;
                                        case 5: s = addDeferredStage(name, scale, once, STAGE_ARG5(inOutputs)); break;
                                        default: s = addDeferredStage(name, scale, once, STAGE_ARG6(inOutputs)); break;
                                        }
                                    }
                                }
                                else
                                    OSG_WARN << "[Pipeline] No output provided: " << name << std::endl;
                            }
                            else if (!inShaders.empty())
                            {
                                s = addDisplayStage(name, inShaders[vIdx], inShaders[fIdx],
                                                    osg::Vec4(0.0f, 0.0f, 1.0f, 1.0f));
                            }
                            else
                                OSG_WARN << "[Pipeline] Invalid stage data: " << name << std::endl;
                            if (!s) continue;

                            // TODO: add other shaders..

                            // Add inputs and uniforms
                            for (size_t n = 0; n < inputs.size(); ++n)
                            {
                                std::string iName = inputs[n].get("name").to_str();
                                int unit = inputs[n].contains("unit") ? inputs[n].get("unit").get<double>() : 0;
                                if (inputs[n].contains("stage"))
                                {
                                    std::string iStage = inputs[n].get("stage").to_str();
                                    Stage* s0 = getStage(iStage);
                                    if (s0 != NULL)
                                    {
                                        std::string iName2 = inputs[n].contains("sampler_name")
                                                           ? inputs[n].get("sampler_name").to_str() : "";
                                        if (!iName2.empty()) s->applyBuffer(*s0, iName, iName2, unit);
                                        else s->applyBuffer(*s0, iName, unit);
                                    }
                                    else
                                    {
                                        osg::NodeCallback* cb = getModule(iStage);
                                        if (cb != NULL)
                                        {
                                            LightModule* light = dynamic_cast<LightModule*>(cb);
                                            if (light) light->applyTextureAndUniforms(s, iName, unit);

                                            ShadowModule* shadow = dynamic_cast<ShadowModule*>(cb);
                                            if (shadow) shadow->applyTextureAndUniforms(s, iName, unit);
                                        }
                                        else
                                            OSG_WARN << "[Pipeline] No such stage " << iStage << "\n";
                                    }
                                }
                                else
                                {
                                    osg::ref_ptr<osg::Texture> t = NULL;
                                    if (sharedTextures.find(iName) == sharedTextures.end()) t = osgVerseUtils::loadTexture(inputs[n], false, sharedIblData);
                                    else t = sharedTextures[iName];
                                    if (!t) { OSG_WARN << "[Pipeline] No such texture " << iName << "\n"; continue; }
                                    else s->applyTexture(t.get(), iName, unit);
                                }
                            }

                            for (size_t n = 0; n < uniforms.size(); ++n)
                            {
                                std::string uName = uniforms[n].get("name").to_str();
                                osg::ref_ptr<osg::Uniform> u = NULL;
                                if (sharedUniforms.find(uName) == sharedUniforms.end()) u = osgVerseUtils::loadUniform(uniforms[n]);
                                else u = sharedUniforms[uName];
                                if (!u) { OSG_WARN << "[Pipeline] No such uniform " << uName << "\n"; continue; }
                                else s->applyUniform(u.get());
                            }
                        }
                    }
                }  // for (size_t i = 0; i < ppArray.size(); ++i)

                applyStagesToView(view, mainCam, forwardMask);
                for (size_t i = 0; i < inputStages.size(); ++i)
                    requireDepthBlit(inputStages[i], true);
            }
        }
        else
            OSG_WARN << "[Pipeline] Unable to load pipeline preset: " << err << std::endl;
        return false;
    }
}
