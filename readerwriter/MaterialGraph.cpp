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
                                                     MaterialPin* pin, int id, bool findFrom)
{
    for (size_t i = 0; i < links.size(); ++i)
    {
        MaterialLink* link = links[i].get();
        if (findFrom) { if (link->nodeTo == node && link->pinTo == pin && link->idTo == id) return link; }
        else { if (link->nodeFrom == node && link->pinFrom == pin && link->idFrom == id) return link; }
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
                    std::string attr = n.contains("attributes") ? n.get("attributes").get<std::string>() : "";
                    picojson::value inJson = n.contains("inputs") ? n.get("inputs") : picojson::value();
                    picojson::value outJson = n.contains("outputs") ? n.get("outputs") : picojson::value();
                    picojson::value imgJson = n.contains("image") ? n.get("image") : picojson::value();
                    if (name.empty() || type.empty()) continue;

                    MaterialNode* node = new MaterialNode;
                    node->name = name; node->type = type; node->id = idCount++;
                    if (!attr.empty())
                    {
                        std::vector<std::string> attrList; osgDB::split(attr, attrList, ';');
                        for (size_t s = 0; s < attrList.size(); ++s)
                        {
                            std::string key = attrList[s]; picojson::value v = n.contains(key) ? n.get(key) : picojson::value();
                            if (v.is<std::string>()) node->attributes[key] = v.get<std::string>();
                        }
                    }
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
                            int index = (int)p.contains("id") ? p.get("id").get<double>() : -1;
                            std::string name = p.contains("name") ? p.get("name").get<std::string>() : "";
                            std::string type = p.contains("type") ? p.get("type").get<std::string>() : "";
                            picojson::value def = p.contains("default_value") ? p.get("default_value") : picojson::value();

                            MaterialPin* pin = new MaterialPin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
                            pin->id = idPinCount++; node->inputs[name][index] = pin;
                        }
                    }

                    if (outJson.is<picojson::array>())
                    {
                        picojson::array outPins = outJson.get<picojson::array>();
                        for (size_t j = 0; j < outPins.size(); ++j)
                        {
                            picojson::value p = outPins[j];
                            int index = (int)p.contains("id") ? p.get("id").get<double>() : -1;
                            std::string name = p.contains("name") ? p.get("name").get<std::string>() : "";
                            std::string type = p.contains("type") ? p.get("type").get<std::string>() : "";
                            picojson::value def = p.contains("default_value") ? p.get("default_value") : picojson::value();

                            MaterialPin* pin = new MaterialPin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
                            pin->id = idPinCount++; node->outputs[name][index] = pin;
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
                    int fID = n.contains("from_id") ? (int)n.get("from_id").get<double>() : -1;
                    std::string fn = n.contains("from_node") ? n.get("from_node").get<std::string>() : "";
                    std::string fp = n.contains("from_socket") ? n.get("from_socket").get<std::string>() : "";
                    int tID = n.contains("to_id") ? (int)n.get("to_id").get<double>() : -1;
                    std::string tn = n.contains("to_node") ? n.get("to_node").get<std::string>() : "";
                    std::string tp = n.contains("to_socket") ? n.get("to_socket").get<std::string>() : "";

                    MaterialNode *nodeF = nodes[fn].get(), *nodeT = nodes[tn].get();
                    if (!nodeF || !nodeT) { OSG_WARN << "[MaterialGraph] Missing node: " << i << std::endl; continue; }

                    MaterialPin *pinF = nodeF->outputs[fp][fID].get(), *pinT = nodeT->inputs[tp][tID].get();
                    if (!pinF || !pinT) { OSG_WARN << "[MaterialGraph] Missing pin: " << i << std::endl; continue; }

                    MaterialLink* link = new MaterialLink; links.push_back(link);
                    link->idFrom = fID; link->nodeFrom = nodeF; link->pinFrom = pinF;
                    link->idTo = tID; link->nodeTo = nodeT; link->pinTo = pinT;
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
    comp.glslFuncs = "#include \"material_nodes.module.glsl\"\n";
    /* MIX: vec3 mixColor/..(vec3 col0, vec3 col1, vec3 f);
     * NORMAL_MAP: vec3 normalStrength(vec3 col, vec3 s);
     * LIGHT_FALLOFF: vec3 lightFalloff(vec3 power, vec3 sm);
     * BRIGHTCONTRAST: vec3 brightnessContrast(vec3 col, vec3 b, vec3 c);
     * HUE_SAT: vec3 setHsv(vec3 col, vec3 fac, vec3 h, vec3 s, vec3 v);
     * RGBTOBW: vec3 setBlackWhite(vec3 col);
     * INVERT: vec3 invert(vec3 col, vec3 fac);
     * GAMMA: vec3 gamma(vec3 col, vec3 g);
     * MATH: vec3 mathAdd/..(vec3 v0, vec3 v1, vec3 v2);
     */

    int glVer = 0; int glslVer = ShaderLibrary::guessShaderVersion(glVer);
    std::string vsCode = "VERSE_VS_OUT vec2 uv;\nvoid main() {\n"
                         "  uv = osg_MultiTexCoord0.st;\n  gl_Position = VERSE_MATRIX_MVP * osg_Vertex;\n}\n";
    for (size_t i = 0; i < endLinks.size(); ++i)
    {
        MaterialLink* eL = endLinks[i]; std::stringstream fsShader;
        std::string str = comp.variable(eL->nodeFrom.get(), eL->pinFrom.get());
        if (eL->pinTo->name == "Metallic" || eL->pinTo->name == "Roughness") continue;  // TODO

        comp.glslGlobal = "VERSE_FS_IN vec2 uv;\nVERSE_FS_OUT vec4 fragData;\n";
        comp.glslVars = "vec4 texColorBridge = vec4(0.0);\n";
        comp.glslCode = "fragData = vec4(" + str + ", texColorBridge.a);\nVERSE_FS_FINAL(fragData);\n";
        comp.bsdfChannelName = eL->pinTo->name;

        // Render a new image using shaders generated by given material graph
        comp.stateset = new osg::StateSet;
        comp.stateset->setTextureAttributeAndModes(0, createDefaultTexture(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f)));
        processBlenderLink(comp, ss, eL->nodeFrom.get(), eL->pinFrom.get(), eL->idFrom);
        fsShader << comp.glslGlobal << comp.glslFuncs << std::string("///////////////////\n")
                 << "void main() {\n" << comp.glslVars << comp.glslCode << "}\n";
        //std::cout << "{{{" << eL->pinTo->name << "}}}\n" << fsShader.str() << std::endl;

        osg::ref_ptr<osg::Program> prog = new osg::Program;
        {
            osg::ref_ptr<osg::Shader> vs = new osg::Shader(osg::Shader::VERTEX, vsCode);
            osg::ref_ptr<osg::Shader> fs = new osg::Shader(osg::Shader::FRAGMENT, fsShader.str());
            prog->addShader(vs.get()); prog->addShader(fs.get());
            Pipeline::createShaderDefinitions(vs.get(), glVer, glslVer);
            Pipeline::createShaderDefinitions(fs.get(), glVer, glslVer);
        }
        comp.stateset->setAttributeAndModes(prog.get());

        unsigned int s = 1024, t = 1024, texUnit = getBlenderTextureUnit(comp);
        osg::Texture* tex = static_cast<osg::Texture*>(ss.getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
        if (tex && tex->getImage(0)) { s = tex->getImage(0)->s(); t = tex->getImage(0)->t(); }

        osg::ref_ptr<osg::Image> result = createShadingResult(*comp.stateset, (s < 16) ? 16 : s, (t < 16) ? 16 : t);
        if (s < 16 || t < 16) result->scaleImage(s, t, 1);
        if (tex) tex->setImage(0, new osg::Image(*result));
    }
}

void MaterialGraph::processBlenderLink(BlenderComposition& comp, const osg::StateSet& ss,
                                       MaterialNode* lastNode, MaterialPin* lastOutPin, int lastOutID)
{
    std::string dst = comp.variable(lastNode, lastOutPin);
    if (comp.glslVars.find("vec3 " + dst) == std::string::npos)
        comp.prependVariables("vec3 " + dst + " = vec3(" + setFromValues(lastOutPin->values) + ");\n");

    typedef MaterialPinIndices::iterator MaterialPinIt;
    if (lastNode->type == "BSDF_PRINCIPLED") {}
    else if (lastNode->type == "TEX_IMAGE")
    {
        /*"inputs": [ { "id": 0, "name": "Vector", "type": "VECTOR", "default_value": [0.0, 0.0, 0.0] } ],
        "outputs": [ { "id": 0, "name": "Color", "type": "RGBA", "default_value": [0.8, 0.8, 0.8, 1.0] },
                     { "id": 1, "name": "Alpha", "type": "VALUE", "default_value": 0.0 } ]*/
        std::string vTex = comp.textureVariable(lastNode), sTex = comp.sampler(lastNode);
        if (comp.glslGlobal.find("uniform sampler2D " + sTex) == std::string::npos)
        {
            comp.prependGlobal("uniform sampler2D " + sTex + ";  // " +
                               comp.bsdfChannelName + ": " + lastNode->imageName + "\n");
        }

        std::string texSetter = (lastOutPin->name == "Alpha") ? "texColorBridge.aaa;\n" : "texColorBridge.rgb;\n";
        comp.prependCode("texColorBridge = VERSE_TEX2D(" + sTex + ", uv);\n" + dst + "=" + texSetter);
        applyBlenderTexture(comp, ss, sTex);
        // TODO: continue find prior nodes?
    }
    else if (lastNode->type == "MIX")
    {
        /*"inputs": [ { "id": 0, "name": "Factor", "type": "VALUE", "default_value": 0.5 },
                      { "id": 1, "name": "Factor", "type": "VECTOR", "default_value": [0.5, 0.5, 0.5] },
                      { "id": 2/4/6/8, "name": "A", "type": "VALUE/VECTOR/RGBA/ROTATION" },
                      { "id": 3/5/7/9, "name": "B", "type": "VALUE/VECTOR/RGBA/ROTATION" } ],
        "outputs": [ { "id": 0/1/2/3, "name": "Result", "type": "VALUE/VECTOR/RGBA/ROTATION" } ]*/
        std::string dataType = lastNode->attributes["data_type"], blendType = lastNode->attributes["blend_type"];
        MaterialPinIt fItr = lastNode->findPin(false, "Factor", (dataType == "VECTOR" ? dataType : "VALUE"));
        MaterialPinIt aItr = lastNode->findPin(false, "A", dataType);
        MaterialPinIt bItr = lastNode->findPin(false, "B", dataType);
        MaterialPin *factor = fItr->second.get(), *inColorA = aItr->second.get(), *inColorB = bItr->second.get();
        std::string factorV = comp.variable(lastNode, factor),
                    color0V = comp.variable(lastNode, inColorA), color1V = comp.variable(lastNode, inColorB);

        comp.prependVariables("vec3 " + factorV + " = vec3(" + setFromValues(factor->values) + ");\n" +
                              "vec3 " + color0V + " = vec3(" + setFromValues(inColorA->values) + ");\n" +
                              "vec3 " + color1V + " = vec3(" + setFromValues(inColorB->values) + ");\n");
        if (blendType == "MIX")
            comp.prependCode(dst + " = mixColor(" + color0V + ", " + color1V + ", " + factorV + ");\n");
        else
            OSG_NOTICE << "[MaterialGraph] Unsupported blending type: " << blendType << std::endl;
        findAndProcessBlenderLink(comp, ss, lastNode, factor, fItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColorA, aItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColorB, bItr->first, true);
    }
    else if (lastNode->type == "NORMAL_MAP")
    {
        MaterialPinIt sItr = lastNode->findPin(false, "Strength"); MaterialPin* strength = sItr->second.get();
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string strV = comp.variable(lastNode, strength), colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + strV + " = vec3(" + setFromValues(strength->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = normalStrength(" + colorV + ", " + strV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, strength, sItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "LIGHT_FALLOFF")
    {
        MaterialPinIt sItr = lastNode->findPin(false, "Strength"); MaterialPin* strength = sItr->second.get();
        MaterialPinIt mItr = lastNode->findPin(false, "Smooth"); MaterialPin* smooth = mItr->second.get();
        std::string strV = comp.variable(lastNode, strength), smV = comp.variable(lastNode, smooth);

        comp.prependVariables("vec3 " + strV + " = vec3(" + setFromValues(strength->values) + ");\n" +
                              "vec3 " + smV + " = vec3(" + setFromValues(smooth->values) + ");\n");
        comp.prependCode(dst + " = lightFalloff(" + strV + ", " + smV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, strength, sItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, smooth, mItr->first, true);
    }
    else if (lastNode->type == "BRIGHTCONTRAST")
    {
        MaterialPinIt bItr = lastNode->findPin(false, "Bright"); MaterialPin* bright = bItr->second.get();
        MaterialPinIt tItr = lastNode->findPin(false, "Contrast"); MaterialPin* contrast = tItr->second.get();
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string brightV = comp.variable(lastNode, bright), contrastV = comp.variable(lastNode, contrast),
                    colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + brightV + " = vec3(" + setFromValues(bright->values) + ");\n" +
                              "vec3 " + contrastV + " = vec3(" + setFromValues(contrast->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = brightnessContrast(" + colorV + ", " + brightV + ", " + contrastV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, bright, bItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, contrast, tItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "HUE_SAT")
    {
        MaterialPinIt fItr = lastNode->findPin(false, "Fac"); MaterialPin* fac = fItr->second.get();
        MaterialPinIt hItr = lastNode->findPin(false, "Hue"); MaterialPin* hue = hItr->second.get();
        MaterialPinIt sItr = lastNode->findPin(false, "Saturation"); MaterialPin* sat = sItr->second.get();
        MaterialPinIt vItr = lastNode->findPin(false, "Value"); MaterialPin* value = vItr->second.get();
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string facV = comp.variable(lastNode, fac), hueV = comp.variable(lastNode, hue), satV = comp.variable(lastNode, sat),
                    valV = comp.variable(lastNode, value), colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + facV + " = vec3(" + setFromValues(fac->values) + ");\n" +
                              "vec3 " + hueV + " = vec3(" + setFromValues(hue->values) + ");\n" +
                              "vec3 " + satV + " = vec3(" + setFromValues(sat->values) + ");\n" +
                              "vec3 " + valV + " = vec3(" + setFromValues(value->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = setHsv(" + colorV + ", " + facV + ", " + hueV + ", " + satV + ", " + valV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, fac, fItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, hue, hItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, sat, sItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, value, vItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "RGBTOBW")
    {
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string colorV = comp.variable(lastNode, inColor);
        comp.prependVariables("vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = setBlackWhite(" + colorV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "SEPARATE_COLOR")
    {
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string colorV = comp.variable(lastNode, inColor);
        std::string setter = (lastOutPin->name == "Red") ? ".rrr" : (lastOutPin->name == "Green" ? ".ggg" : ".bbb");

        comp.prependVariables("vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = " + colorV + setter + ";\n");
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "INVERT")
    {
        MaterialPinIt fItr = lastNode->findPin(false, "Fac"); MaterialPin* fac = fItr->second.get();
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string facV = comp.variable(lastNode, fac), colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + facV + " = vec3(" + setFromValues(fac->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = invert(" + colorV + ", " + facV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, fac, fItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "GAMMA")
    {
        MaterialPinIt gItr = lastNode->findPin(false, "Gamma"); MaterialPin* gamma = gItr->second.get();
        MaterialPinIt cItr = lastNode->findPin(false, "Color"); MaterialPin* inColor = cItr->second.get();
        std::string gammaV = comp.variable(lastNode, gamma), colorV = comp.variable(lastNode, inColor);

        comp.prependVariables("vec3 " + gammaV + " = vec3(" + setFromValues(gamma->values) + ");\n" +
                              "vec3 " + colorV + " = vec3(" + setFromValues(inColor->values) + ");\n");
        comp.prependCode(dst + " = gamma(" + colorV + ", " + gammaV + ");\n");
        findAndProcessBlenderLink(comp, ss, lastNode, gamma, gItr->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, inColor, cItr->first, true);
    }
    else if (lastNode->type == "MATH")
    {
        std::string operation = lastNode->attributes["operation"];
        MaterialPinIt it = lastNode->inputs["Value"].begin();
        MaterialPinIt vItr0 = it++; MaterialPinIt vItr1 = it++; MaterialPinIt vItr2 = it++;
        MaterialPin *v0 = vItr0->second.get(), *v1 = vItr1->second.get(), *v2 = vItr2->second.get();

        std::string val0 = comp.variable(lastNode, v0), val1 = comp.variable(lastNode, v1), val2 = comp.variable(lastNode, v2);
        comp.prependVariables("vec3 " + val0 + " = vec3(" + setFromValues(v0->values) + ");\n" +
                              "vec3 " + val1 + " = vec3(" + setFromValues(v1->values) + ");\n" +
                              "vec3 " + val2 + " = vec3(" + setFromValues(v2->values) + ");\n");
        if (operation == "ADD")
            comp.prependCode(dst + " = mathAdd(" + val0 + ", " + val1 + ", " + val2 + ");\n");
        else if (operation == "SUBTRACT")
            comp.prependCode(dst + " = mathSub(" + val0 + ", " + val1 + ", " + val2 + ");\n");
        else if (operation == "MULTIPLY")
            comp.prependCode(dst + " = mathMult(" + val0 + ", " + val1 + ", " + val2 + ");\n");
        else
            OSG_NOTICE << "[MaterialGraph] Unsupported math-operation: " << operation << std::endl;
        findAndProcessBlenderLink(comp, ss, lastNode, v0, vItr0->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, v1, vItr1->first, true);
        findAndProcessBlenderLink(comp, ss, lastNode, v2, vItr2->first, true);
    }
    else
    {
        OSG_WARN << "[MaterialGraph] Unsupported link node type: " << lastNode->type << std::endl;
        for (MaterialPinMap::iterator itr = lastNode->inputs.begin(); itr != lastNode->inputs.end(); ++itr)
        {
            for (MaterialPinIndices::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
                OSG_WARN << "\tInput pin: " << itr->first << ": " << itr2->second->type << ", ID = " << itr2->first << std::endl;
        }
    }
}

void MaterialGraph::findAndProcessBlenderLink(BlenderComposition& comp, const osg::StateSet& ss,
                                              MaterialNode* node, MaterialPin* pin, int id, bool findFrom)
{
    MaterialLink* link = findLink(comp.links, node, pin, id, findFrom);
    if (!link)
    {
        OSG_INFO << "[MaterialGraph] No further links: " << node->name << ", " << pin->name << std::endl;
        return;
    }

    if (findFrom)
    {
        std::string varFrom = comp.variable(link->nodeFrom.get(), link->pinFrom.get());
        std::string varTo = comp.variable(node, pin);
        comp.prependCode(varTo + " = " + varFrom + ";\n");
        processBlenderLink(comp, ss, link->nodeFrom.get(), link->pinFrom.get(), link->idFrom);
    }
    else
    {
        std::string varFrom = comp.variable(node, pin);
        std::string varTo = comp.variable(link->nodeTo.get(), link->pinTo.get());
        comp.prependCode(varTo + " = " + varFrom + ";\n");
        processBlenderLink(comp, ss, link->nodeTo.get(), link->pinTo.get(), link->idTo);
    }
}

void MaterialGraph::applyBlenderTexture(BlenderComposition& comp, const osg::StateSet& ss, const std::string& samplerName)
{
    unsigned int texUnit = getBlenderTextureUnit(comp);
    const osg::Texture* tex = static_cast<const osg::Texture*>(ss.getTextureAttribute(texUnit, osg::StateAttribute::TEXTURE));
    if (tex != NULL)
        comp.stateset->setTextureAttributeAndModes(0, static_cast<osg::Texture*>(tex->clone(osg::CopyOp::SHALLOW_COPY)));
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
