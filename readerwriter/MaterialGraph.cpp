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
    /* Reference:
       * Documents: https://docs.blender.org/manual/en/latest/compositing/index.html
       * NameDefinitions: https://github.com/blender/blender/blob/main/source/blender/blenkernel/BKE_node_legacy_types.hh
       * Implementations:  https://github.com/blender/blender/tree/main/source/blender/nodes/shader/nodes
     * -- COLOR ADJUSTER NODES --
       * Brightness/Contrast (BRIGHTCONTRAST): vec3 brightnessContrast(vec3 col, vec3 b, vec3 c);
       * Color Balance (TODO)
       * Color Correction (TODO)
       * Exposure (TODO)
       * Gamma (GAMMA): vec3 gamma(vec3 col, vec3 g);
       * Hue Correct (TODO)
       * Hue/Saturation/Value (HUE_SAT): vec3 setHsv(vec3 col, vec3 fac, vec3 h, vec3 s, vec3 v);
       * RGB Curves (TODO)
       * Tone Map (TODO)
     * -- COLOR MIX NODES --
       * Alpha Over (TODO)
       * Combine Color (TODO)
       * Separate Color (SEPARATE_COLOR)
       * Mix Color (MIX): vec3 mixColor/..(vec3 col0, vec3 col1, vec3 f);
       * Z Combine (TODO)
     * -- COLOR NODES --
       * Alpha Convert (TODO)
       * Blackbody (TODO)
       * Color Ramp (TODO)
       * Convert Colorspace (TODO)
       * Set Alpha (TODO)
       * Invert Color (INVERT): vec3 invert(vec3 col, vec3 fac);
       * RGB to BW (RGBTOBW): vec3 setBlackWhite(vec3 col);
     * -- Unsorted --
     * NORMAL_MAP: vec3 normalStrength(vec3 col, vec3 s);
     * LIGHT_FALLOFF: vec3 lightFalloff(vec3 power, vec3 sm);
     * MATH: vec3 mathAdd/..(vec3 v0, vec3 v1, vec3 v2);
     * VECT_MATH: vec3 mathAdd/..(vec3 v0, vec3 v1, vec3 v2);
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

#define NEW_INPUT_PIN_EX(N, N2) \
    MaterialPinIt itr_##N = lastNode->findPin(false, #N, N2); MaterialPin* pin_##N = (itr_##N)->second.get(); \
    std::string var_##N = comp.variable(lastNode, pin_##N);
#define NEW_INPUT_SUBPIN(N, O) \
    MaterialPinIt itr_##N##O = lastNode->inputs[#N].begin(); for (int i = 0; i < O; ++i) itr_##N##O ++; \
    MaterialPin* pin_##N##O = (itr_##N##O)->second.get(); std::string var_##N##O = comp.variable(lastNode, pin_##N##O);
#define NEW_INPUT_PIN(N) NEW_INPUT_PIN_EX(N, "")
#define PROCESS_PIN(N) findAndProcessBlenderLink(comp, ss, lastNode, pin_##N, (itr_##N)->first, true);
#define PROCESS_SUBPIN(N, O) findAndProcessBlenderLink(comp, ss, lastNode, pin_##N##O, (itr_##N##O)->first, true);
#define GLSL_CODE(code) comp.prependCode(dst + " = " + std::string(code) + ";\n");
#define GLSL_VARIABLE(T, N) comp.prependVariables(#T " " + var_##N + " = " #T "(" + setFromValues(pin_##N->values) + ");\n");
#define GLSL_VARIABLE3(N) GLSL_VARIABLE(vec3, N)

    typedef MaterialPinIndices::iterator MaterialPinIt;
    if (lastNode->type == "BSDF_PRINCIPLED") {}
    // INPUT NODES
    else if (lastNode->type == "TEX_IMAGE")
    {
        std::string vTex = comp.textureVariable(lastNode), sTex = comp.sampler(lastNode);
        if (comp.glslGlobal.find("uniform sampler2D " + sTex) == std::string::npos)
        {
            comp.prependGlobal("uniform sampler2D " + sTex + ";  // " +
                               comp.bsdfChannelName + ": " + lastNode->imageName + "\n");
        }

        std::string texSetter = (lastOutPin->name == "Alpha") ? "texColorBridge.aaa;\n" : "texColorBridge.rgb;\n";
        comp.prependCode("texColorBridge = VERSE_TEX2D(" + sTex + ", uv);\n" + dst + "=" + texSetter);
        applyBlenderTexture(comp, ss, sTex);  // TODO: continue find prior nodes?
    }
    // COLOR ADJUST NODES
    else if (lastNode->type == "BRIGHTCONTRAST")
    {
        NEW_INPUT_PIN(Bright); NEW_INPUT_PIN(Contrast); NEW_INPUT_PIN(Color);
        GLSL_VARIABLE3(Bright); GLSL_VARIABLE3(Contrast); GLSL_VARIABLE3(Color);
        GLSL_CODE("brightnessContrast(" + var_Color + ", " + var_Bright + ", " + var_Contrast + ")");
        PROCESS_PIN(Bright); PROCESS_PIN(Contrast); PROCESS_PIN(Color);
    }
    else if (lastNode->type == "GAMMA")
    {
        NEW_INPUT_PIN(Gamma); NEW_INPUT_PIN(Color);
        GLSL_VARIABLE3(Gamma); GLSL_VARIABLE3(Color);
        GLSL_CODE("gamma(" + var_Color + ", " + var_Gamma + ")");
        PROCESS_PIN(Gamma); PROCESS_PIN(Color);
    }
    else if (lastNode->type == "HUE_SAT")
    {
        NEW_INPUT_PIN(Fac); NEW_INPUT_PIN(Hue); NEW_INPUT_PIN(Saturation); NEW_INPUT_PIN(Value); NEW_INPUT_PIN(Color);
        GLSL_VARIABLE3(Fac); GLSL_VARIABLE3(Hue); GLSL_VARIABLE3(Saturation); GLSL_VARIABLE3(Value); GLSL_VARIABLE3(Color);
        GLSL_CODE("setHsv(" + var_Color + ", " + var_Fac + ", " + var_Hue + ", " + var_Saturation + ", " + var_Value + ")");
        PROCESS_PIN(Fac); PROCESS_PIN(Hue); PROCESS_PIN(Saturation); PROCESS_PIN(Value); PROCESS_PIN(Color);
    }
    // COLOR MIX NODES
    else if (lastNode->type == "MIX")
    {
        std::string dataType = lastNode->attributes["data_type"], blendType = lastNode->attributes["blend_type"];
        NEW_INPUT_PIN_EX(Factor, (dataType == "VECTOR" ? dataType : "VALUE"));
        NEW_INPUT_PIN_EX(A, dataType); NEW_INPUT_PIN_EX(B, dataType);
        GLSL_VARIABLE3(Factor); GLSL_VARIABLE3(A); GLSL_VARIABLE3(B);
        if (blendType == "MIX")
            { GLSL_CODE("mixColor(" + var_A + ", " + var_B + ", " + var_Factor + ")"); }
        else
            { OSG_NOTICE << "[MaterialGraph] Unsupported blending type: " << blendType << std::endl; }
        PROCESS_PIN(Factor); PROCESS_PIN(A); PROCESS_PIN(B);
    }
    else if (lastNode->type == "SEPARATE_COLOR")
    {
        std::string setter = (lastOutPin->name == "Red") ? ".rrr" : (lastOutPin->name == "Green" ? ".ggg" : ".bbb");
        NEW_INPUT_PIN(Color); GLSL_VARIABLE3(Color); GLSL_CODE(var_Color + setter); PROCESS_PIN(Color);
    }
    // COLOR NODES
    else if (lastNode->type == "INVERT")
    {
        NEW_INPUT_PIN(Fac); NEW_INPUT_PIN(Color); GLSL_VARIABLE3(Fac); GLSL_VARIABLE3(Color);
        GLSL_CODE("invert(" + var_Color + ", " + var_Fac + ")"); PROCESS_PIN(Fac); PROCESS_PIN(Color);
    }
    else if (lastNode->type == "RGBTOBW")
    {
        NEW_INPUT_PIN(Color); GLSL_VARIABLE3(Color);
        GLSL_CODE("setBlackWhite(" + var_Color + ")"); PROCESS_PIN(Color);
    }
    /// UNSORTED
    else if (lastNode->type == "NORMAL_MAP")
    {
        NEW_INPUT_PIN(Strength); NEW_INPUT_PIN(Color); GLSL_VARIABLE3(Strength); GLSL_VARIABLE3(Color);
        GLSL_CODE("normalStrength(" + var_Color + ", " + var_Strength + ")");
        PROCESS_PIN(Strength); PROCESS_PIN(Color);
    }
    else if (lastNode->type == "LIGHT_FALLOFF")
    {
        NEW_INPUT_PIN(Strength); NEW_INPUT_PIN(Smooth); GLSL_VARIABLE3(Strength); GLSL_VARIABLE3(Smooth);
        GLSL_CODE("lightFalloff(" + var_Strength + ", " + var_Smooth + ")");
        PROCESS_PIN(Strength); PROCESS_PIN(Smooth);
    }
    else if (lastNode->type == "MATH")
    {
        std::string operation = lastNode->attributes["operation"];
        NEW_INPUT_SUBPIN(Value, 0); NEW_INPUT_SUBPIN(Value, 1); NEW_INPUT_SUBPIN(Value, 2);
        GLSL_VARIABLE3(Value0); GLSL_VARIABLE3(Value1); GLSL_VARIABLE3(Value2);
        if (operation == "ADD") { GLSL_CODE("mathAdd(" + var_Value0 + ", " + var_Value1 + ", " + var_Value2 + ")"); }
        else if (operation == "SUBTRACT") { GLSL_CODE("mathSub(" + var_Value0 + ", " + var_Value1 + ", " + var_Value2 + ")"); }
        else if (operation == "MULTIPLY") { GLSL_CODE("mathMult(" + var_Value0 + ", " + var_Value1 + ", " + var_Value2 + ")"); }
        else { OSG_NOTICE << "[MaterialGraph] Unsupported math-operation: " << operation << std::endl; }
        PROCESS_SUBPIN(Value, 0); PROCESS_SUBPIN(Value, 1); PROCESS_SUBPIN(Value, 2);
    }
    else if (lastNode->type == "VECT_MATH")
    {
        std::string operation = lastNode->attributes["operation"];
        NEW_INPUT_SUBPIN(Vector, 0); NEW_INPUT_SUBPIN(Vector, 1); NEW_INPUT_SUBPIN(Vector, 2);
        GLSL_VARIABLE3(Vector0); GLSL_VARIABLE3(Vector1); GLSL_VARIABLE3(Vector2);
        if (operation == "ADD") { GLSL_CODE("mathAdd(" + var_Vector0 + ", " + var_Vector1 + ", " + var_Vector2 + ")"); }
        else if (operation == "SUBTRACT") { GLSL_CODE("mathSub(" + var_Vector0 + ", " + var_Vector1 + ", " + var_Vector2 + ")"); }
        else if (operation == "MULTIPLY") { GLSL_CODE("mathMult(" + var_Vector0 + ", " + var_Vector1 + ", " + var_Vector2 + ")"); }
        else { OSG_NOTICE << "[MaterialGraph] Unsupported vector math-operation: " << operation << std::endl; }
        PROCESS_SUBPIN(Vector, 0); PROCESS_SUBPIN(Vector, 1); PROCESS_SUBPIN(Vector, 2);
    }
    else
    {
        OSG_WARN << "[MaterialGraph] Unsupported link node type: " << lastNode->type << std::endl;
        for (auto attr : lastNode->attributes) OSG_WARN << "\tAttribute: " << attr.first << " = " << attr.second << std::endl;
        for (MaterialPinMap::iterator itr = lastNode->inputs.begin(); itr != lastNode->inputs.end(); ++itr)
            for (MaterialPinIndices::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            { OSG_WARN << "\tInput pin: " << itr->first << ": " << itr2->second->type << ", ID = " << itr2->first << std::endl; }
        for (MaterialPinMap::iterator itr = lastNode->outputs.begin(); itr != lastNode->outputs.end(); ++itr)
            for (MaterialPinIndices::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); ++itr2)
            { OSG_WARN << "\tOutput pin: " << itr->first << ": " << itr2->second->type << ", ID = " << itr2->first << std::endl; }
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
