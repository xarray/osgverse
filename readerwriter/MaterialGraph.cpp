#include <picojson.h>
#include <osg/io_utils>
#include <osgDB/WriteFile>
#include "pipeline/Utilities.h"
#include "pipeline/ShaderLibrary.h"
#include "pipeline/Pipeline.h"
#include "MaterialGraph.h"
using namespace osgVerse;

static std::vector<double> readFromValues(picojson::value& value)
{
    std::vector<double> values;
    if (value.is<picojson::array>())
    {
        picojson::array vList = value.get<picojson::array>();
        for (size_t i = 0; i < vList.size(); ++i)
        { if (vList[i].is<double>()) values.push_back(vList[i].get<double>()); }
    }
    else if (value.is<double>())
        values.push_back(value.get<double>());
    return values;
}

static std::string setFromValues(const std::vector<double>& values, unsigned int num = 3)
{
    unsigned int size = values.size();
    std::stringstream ss; if (size == 0) return "0.0";
    for (unsigned int i = 0; i < num; ++i)
    { if (i > 0) ss << ","; ss << (i < size ? values[i] : values[0]); }
    return ss.str();
}

MaterialGraph* MaterialGraph::instance()
{
    static osg::ref_ptr<MaterialGraph> s_instance = new MaterialGraph;
    return s_instance.get();
}

MaterialGraph::MaterialLink* MaterialGraph::findLink(MaterialLinkList& links, MaterialNode* node,
                                                     MaterialPin* pin, bool findFrom)
{
    for (size_t i = 0; i < links.size(); ++i)
    {
        MaterialLink* link = links[i].get();
        if (findFrom) { if (link->nodeTo == node && link->pinTo == pin) return link; }
        else { if (link->nodeFrom == node && link->pinFrom == pin) return link; }
    }
    return NULL;
}

bool MaterialGraph::readFromBlender(const std::string& data, osg::StateSet& ss)
{
    picojson::value dataRoot, graphRoot;
    std::string err = picojson::parse(dataRoot, data);
    if (!err.empty())
    {
        OSG_WARN << "[MaterialGraph] Failed to read Blender graph data: "
                 << err << std::endl; return false;
    }

    unsigned int idCount = 0, idPinCount = 0;
    if (dataRoot.contains("VERSE_material_graph"))
    {
        std::string graphString = dataRoot.get("VERSE_material_graph").get<std::string>();
        err = picojson::parse(graphRoot, graphString);
        if (!err.empty())
        {
            OSG_WARN << "[MaterialGraph] Failed to load 'VERSE_material_graph' from Blender graph: "
                     << err << std::endl; return false;
        }

        MaterialNodeMap nodes;
        if (graphRoot.contains("nodes"))
        {
            picojson::value nodesJson = graphRoot.get("nodes");
            if (nodesJson.is<picojson::array>())
            {
                picojson::array nodeList = nodesJson.get<picojson::array>();
                for (size_t i = 0; i < nodeList.size(); ++i)
                {
                    picojson::value n = nodeList[i];
                    std::string name = n.contains("name") ? n.get("name").get<std::string>() : "";
                    std::string type = n.contains("type") ? n.get("type").get<std::string>() : "";
                    picojson::value inJson = n.contains("inputs") ? n.get("inputs") : picojson::value();
                    picojson::value outJson = n.contains("outputs") ? n.get("outputs") : picojson::value();
                    picojson::value imgJson = n.contains("image") ? n.get("image") : picojson::value();
                    if (name.empty() || type.empty()) continue;

                    MaterialNode* node = new MaterialNode;
                    node->name = name; node->type = type; node->id = idCount++;
                    if (!imgJson.is<picojson::null>())
                    {
                        if (imgJson.contains("name")) node->imageName = imgJson.get("name").get<std::string>();
                        if (imgJson.contains("filepath")) node->imagePath = imgJson.get("filepath").get<std::string>();
                    }

                    if (inJson.is<picojson::array>())
                    {
                        picojson::array inPins = inJson.get<picojson::array>();
                        for (size_t j = 0; j < inPins.size(); ++j)
                        {
                            picojson::value p = inPins[j];
                            std::string name = p.contains("name") ? p.get("name").get<std::string>() : "";
                            std::string type = p.contains("type") ? p.get("type").get<std::string>() : "";
                            picojson::value def = p.contains("default_value") ? p.get("default_value") : picojson::value();

                            MaterialPin* pin = new MaterialPin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
                            pin->id = idPinCount++; node->inputs[name] = pin;
                        }
                    }

                    if (outJson.is<picojson::array>())
                    {
                        picojson::array outPins = outJson.get<picojson::array>();
                        for (size_t j = 0; j < outPins.size(); ++j)
                        {
                            picojson::value p = outPins[j];
                            std::string name = p.contains("name") ? p.get("name").get<std::string>() : "";
                            std::string type = p.contains("type") ? p.get("type").get<std::string>() : "";
                            picojson::value def = p.contains("default_value") ? p.get("default_value") : picojson::value();

                            MaterialPin* pin = new MaterialPin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
                            pin->id = idPinCount++; node->outputs[name] = pin;
                        }
                    }
                    nodes[name] = node;
                }
            }
        }

        MaterialLinkList links;
        if (graphRoot.contains("links"))
        {
            picojson::value linksJson = graphRoot.get("links");
            if (linksJson.is<picojson::array>())
            {
                picojson::array linkList = linksJson.get<picojson::array>();
                for (size_t i = 0; i < linkList.size(); ++i)
                {
                    picojson::value n = linkList[i];
                    std::string fn = n.contains("from_node") ? n.get("from_node").get<std::string>() : "";
                    std::string fp = n.contains("from_socket") ? n.get("from_socket").get<std::string>() : "";
                    std::string tn = n.contains("to_node") ? n.get("to_node").get<std::string>() : "";
                    std::string tp = n.contains("to_socket") ? n.get("to_socket").get<std::string>() : "";

                    MaterialNode *nodeF = nodes[fn].get(), *nodeT = nodes[tn].get();
                    if (!nodeF || !nodeT) { OSG_WARN << "[MaterialGraph] Missing node: " << i << std::endl; continue; }

                    MaterialPin *pinF = nodeF->outputs[fp].get(), *pinT = nodeT->inputs[tp].get();
                    if (!pinF || !pinT) { OSG_WARN << "[MaterialGraph] Missing pin: " << i << std::endl; continue; }

                    MaterialLink* link = new MaterialLink; links.push_back(link);
                    link->nodeFrom = nodeF; link->nodeTo = nodeT; link->pinFrom = pinF; link->pinTo = pinT;
                }
            }
        }
        //std::cout << graphRoot.serialize(true) << std::endl;
        processBlenderLinks(nodes, links, ss); return true;
    }
    return false;
}

void MaterialGraph::processBlenderLinks(MaterialNodeMap& nodes, MaterialLinkList& links, osg::StateSet& ss)
{
    std::vector<MaterialLink*> endLinks;
    for (size_t i = 0; i < links.size(); ++i)
    {
        if (links[i]->nodeTo->name.find("BSDF") != std::string::npos)
            endLinks.push_back(links[i].get());
    }

    BlenderComposition comp; comp.links = links;
    comp.glslFuncs = std::string("///////////////////\n") +
                     "vec3 mixColor(vec3 col0, vec3 col1, vec3 f) { return mix(col0, col1, f); }\n" +
                     "vec3 lightFalloff(vec3 power, vec3 sm) { return smoothstep(vec3(0.0), power, sm); }\n" +
                     "vec3 normalStrength(vec3 col, vec3 s) { return pow(col, s); }\n" +
                     "vec3 brightnessContrast(vec3 col, vec3 b, vec3 c) { return mix(vec3(0.5), col * (b + vec3(1.0)), c + vec3(1.0)); }\n";

    int glVer = 0; int glslVer = ShaderLibrary::guessShaderVersion(glVer);
    std::string vsCode = "VERSE_VS_OUT vec2 uv;\nvoid main() {\n"
                         "  uv = osg_MultiTexCoord0.st;\n  gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n}\n";
    osg::ref_ptr<osg::Shader> vs = new osg::Shader(osg::Shader::VERTEX, vsCode);
    Pipeline::createShaderDefinitions(vs.get(), glVer, glslVer);

    for (size_t i = 0; i < endLinks.size(); ++i)
    {
        MaterialLink* eL = endLinks[i]; std::stringstream fsShader;
        std::string str = comp.variable(eL->nodeFrom.get(), eL->pinFrom.get());
        if (eL->pinTo->name == "Normal" || eL->pinTo->name == "Metallic" || eL->pinTo->name == "Roughness") continue;  // TODO

        comp.glslGlobal = "VERSE_FS_IN vec2 uv;\nVERSE_FS_OUT vec4 fragData;\n";
        comp.glslVars = "vec4 texColorBridge = vec4(0.0);\n";
        comp.glslCode = "fragData = vec4(" + str + ", texColorBridge.a);\nVERSE_FS_FINAL(fragData);\n";
        comp.bsdfChannelName = eL->pinTo->name;

        comp.stateset = new osg::StateSet;
        comp.stateset->setTextureAttributeAndModes(0, createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        processBlenderLink(comp, ss, eL->nodeFrom.get(), eL->pinFrom.get());
        fsShader << comp.glslGlobal << comp.glslFuncs << std::string("///////////////////\n")
                 << "void main() {\n" << comp.glslVars << comp.glslCode << "}\n";
        std::cout << "{{{" << eL->pinTo->name << "}}}\n" << fsShader.str() << std::endl;

        // Render a new image using shaders generated by given material graph
        osg::ref_ptr<osg::Shader> fs = new osg::Shader(osg::Shader::FRAGMENT, fsShader.str());
        osg::ref_ptr<osg::Program> prog = new osg::Program;
        prog->addShader(vs.get()); prog->addShader(fs.get());
        Pipeline::createShaderDefinitions(fs.get(), glVer, glslVer);
        comp.stateset->setAttributeAndModes(prog.get());

        unsigned int s = 1024, t = 1024, texUnit = getBlenderTextureUnit(comp);
        osg::Texture* tex = static_cast<osg::Texture*>(ss.getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
        if (tex && tex->getImage(0)) { s = tex->getImage(0)->s(); t = tex->getImage(0)->t(); }

        osg::ref_ptr<osg::Image> result = createShadingResult(*comp.stateset, (s < 16) ? 16 : s, (t < 16) ? 16 : t);
        if (s < 16 || t < 16) result->scaleImage(s, t, 1);
        if (tex) tex->setImage(0, new osg::Image(*result));
    }
}

void MaterialGraph::processBlenderLink(BlenderComposition& comp, osg::StateSet& ss,
                                       MaterialNode* lastNode, MaterialPin* lastOutPin)
{
    std::string dst = comp.variable(lastNode, lastOutPin);
    comp.prependVariables("vec3 " + dst + " = vec3(" + setFromValues(lastOutPin->values) + ");\n");

    if (lastNode->type == "BSDF_PRINCIPLED") {}
    else if (lastNode->type == "TEX_IMAGE")
    {
        std::string vTex = comp.textureVariable(lastNode), sTex = comp.sampler(lastNode);
        comp.prependGlobal("uniform sampler2D " + sTex + ";  // " +
                           comp.bsdfChannelName + ": " + lastNode->imageName + "\n");
        comp.prependCode("texColorBridge = VERSE_TEX2D(" + sTex + ", uv);\n" + dst + " = texColorBridge.rgb;\n");
        applyBlenderTexture(comp, ss, sTex);
        // TODO: continue find prior nodes?
    }
    else if (lastNode->type == "MIX")
    {
        MaterialPin* factor = lastNode->inputs["Factor"].get();
        MaterialPin* inColorA = lastNode->inputs["A"].get();
        MaterialPin* inColorB = lastNode->inputs["B"].get();
        std::string factorV = comp.variable(lastNode, factor),
                    color0V = comp.variable(lastNode, inColorA), color1V = comp.variable(lastNode, inColorB);

        comp.prependVariables("vec3 " + factorV + " = vec3(" + setFromValues(factor->values) + ");\n" +
                              "vec3 " + color0V + " = vec3(" + setFromValues(inColorA->values) + ");\n" +
                              "vec3 " + color1V + " = vec3(" + setFromValues(inColorB->values) + ");\n");
        comp.prependCode(dst + " = mixColor(" + color0V + ", " + color1V + ", " + factorV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, factor, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColorA, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColorB, true);
    }
    else if (lastNode->type == "NORMAL_MAP")
    {
        MaterialPin* strength = lastNode->inputs["Strength"].get();
        MaterialPin* inColor = lastNode->inputs["Color"].get();
        std::string strV = comp.variable(lastNode, strength), colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + strV + " = vec3(" + setFromValues(strength->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = normalStrength(" + colorV + ", " + strV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, strength, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, true);
    }
    else if (lastNode->type == "LIGHT_FALLOFF")
    {
        MaterialPin* strength = lastNode->inputs["Strength"].get();
        MaterialPin* smooth = lastNode->inputs["Smooth"].get();
        std::string strV = comp.variable(lastNode, strength), smV = comp.variable(lastNode, smooth);

        comp.prependVariables("vec3 " + strV + " = vec3(" + setFromValues(strength->values) + ");\n" +
                              "vec3 " + smV + " = vec3(" + setFromValues(smooth->values) + ");\n");
        comp.prependCode(dst + " = lightFalloff(" + strV + ", " + smV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, strength, true);
        findAndProcessBlenderLink(comp, ss, lastNode, smooth, true);
    }
    else if (lastNode->type == "BRIGHTCONTRAST")
    {
        MaterialPin* bright = lastNode->inputs["Bright"].get();
        MaterialPin* contrast = lastNode->inputs["Contrast"].get();
        MaterialPin* inColor = lastNode->inputs["Color"].get();
        std::string brightV = comp.variable(lastNode, bright), contrastV = comp.variable(lastNode, contrast),
                    colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + brightV + " = vec3(" + setFromValues(bright->values) + ");\n" +
                              "vec3 " + contrastV + " = vec3(" + setFromValues(contrast->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = brightnessContrast(" + colorV + ", " + brightV + ", " + contrastV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, bright, true);
        findAndProcessBlenderLink(comp, ss, lastNode, contrast, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, true);
    }
    else
    {
        OSG_NOTICE << "[MaterialGraph] Unsupported link node type: " << lastNode->type << std::endl;
        for (MaterialPinMap::iterator itr = lastNode->inputs.begin(); itr != lastNode->inputs.end(); ++itr)
            OSG_NOTICE << "\tInput pin: " << itr->first << ": " << itr->second->type << std::endl;
    }
}

void MaterialGraph::findAndProcessBlenderLink(BlenderComposition& comp, osg::StateSet& ss,
                                              MaterialNode* node, MaterialPin* pin, bool findFrom)
{
    MaterialLink* link = findLink(comp.links, node, pin, findFrom); if (!link) return;
    if (findFrom)
    {
        std::string varFrom = comp.variable(link->nodeFrom.get(), link->pinFrom.get());
        std::string varTo = comp.variable(node, pin);
        comp.prependCode(varTo + " = " + varFrom + ";\n");
        processBlenderLink(comp, ss, link->nodeFrom.get(), link->pinFrom.get());
    }
    else
    {
        std::string varFrom = comp.variable(node, pin);
        std::string varTo = comp.variable(link->nodeTo.get(), link->pinTo.get());
        comp.prependCode(varTo + " = " + varFrom + ";\n");
        processBlenderLink(comp, ss, link->nodeTo.get(), link->pinTo.get());
    }
}

void MaterialGraph::applyBlenderTexture(BlenderComposition& comp, osg::StateSet& ss, const std::string& samplerName)
{
    unsigned int texUnit = getBlenderTextureUnit(comp);
    comp.stateset->setTextureAttributeAndModes(0, ss.getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
    comp.stateset->getOrCreateUniform(samplerName, osg::Uniform::INT)->set((int)0);
}

unsigned int MaterialGraph::getBlenderTextureUnit(BlenderComposition& comp)
{
    unsigned int texUnit = 0;  // default: base color
    if (comp.bsdfChannelName == "Normal") texUnit = 1;
    else if (comp.bsdfChannelName == "Specular") texUnit = 2;
    else if (comp.bsdfChannelName == "Metallic") texUnit = 3;
    else if (comp.bsdfChannelName == "Roughness") texUnit = 4;  // FIXME
    else if (comp.bsdfChannelName == "Emission") texUnit = 5;
    return texUnit;
}
