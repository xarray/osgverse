#include <picojson.h>
#include <iostream>
#include <sstream>
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

    unsigned int idCount = 0;
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
                    if (!imgJson.is<picojson::null>() && imgJson.contains("filepath"))
                        node->imagePath = imgJson.get("filepath").get<std::string>();

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
                            pin->id = idCount++; node->inputs[name] = pin;
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
                            pin->id = idCount++; node->outputs[name] = pin;
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

        processBlenderLinks(nodes, links, ss);
        //std::cout << graphRoot.serialize(true) << std::endl;
        return true;
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

    std::string glslCode, glslVars, glslGlobal;
    for (size_t i = 0; i < links.size(); ++i)
    {
        MaterialLink* eL = links[i]; std::stringstream str, gShader;
        str << "var_" << eL->nodeFrom->id << "_" << eL->pinFrom->id;

        glslGlobal = "VERSE_FS_IN vec2 uv;\nVERSE_FS_OUT vec4 fragData;\n";
        glslVars = "vec3 " + str.str() + " = vec3(" + setFromValues(eL->pinFrom->values) + ");\n";
        glslCode = "fragData = vec3(" + str.str() + ", 1.0);\nVERSE_FS_FINAL(fragData);\n";
        processBlenderLink(glslCode, glslVars, glslGlobal, links, ss, eL->nodeFrom.get(), eL->pinFrom.get());

        gShader << glslGlobal << "void main() {\n" << glslVars << glslCode << "}\n";
        std::cout << "[" << eL->pinTo->name << "]\n" << gShader.str() << std::endl;  // TODO
    }
}

void MaterialGraph::processBlenderLink(std::string& glslCode, std::string& glslVars, std::string& glslGlobal,
                                       MaterialLinkList& links, osg::StateSet& ss,
                                       MaterialNode* lastNode, MaterialPin* lastOutPin)
{
    std::stringstream dst; dst << "var_" << lastNode->id << "_" << lastOutPin->id;
    if (lastNode->type == "TEX_IMAGE")
    {
        std::stringstream vTex, sTex; vTex << "var_tex" << lastNode->id; sTex << "tex" << lastNode->id;
        glslGlobal = "uniform sampler2D " + sTex.str() + ";\n" + glslGlobal;
        glslVars = "vec4 " + vTex.str() + " = VERSE_TEX2D(" + sTex.str() + ", uv);\n" + glslVars;
        glslCode = dst.str() + " = vec3(" + vTex.str() + ");\n" + glslCode;
        // TODO: continue find prior nodes?
    }
    else if (lastNode->type == "NORMAL_MAP")
    {

    }
    else if (lastNode->type == "BRIGHTCONTRAST")
    {
        MaterialPin* bright = lastNode->inputs["Bright"].get();
        MaterialPin* contrast = lastNode->inputs["Contrast"].get();
        MaterialPin* inColor = lastNode->inputs["Color"].get();

        std::stringstream brightV, contrastV, colorV;
        brightV << "var_" << lastNode->id << "_" << bright->id;
        contrastV << "var_" << lastNode->id << "_" << contrast->id;
        colorV << "var_" << lastNode->id << "_" << inColor->id;

        glslVars = "vec3 " + brightV.str() + " = vec3(" + setFromValues(bright->values) + ");\n"
                 + "vec3 " + contrastV.str() + " = vec3(" + setFromValues(contrast->values) + ");\n"
                 + "vec3 " + colorV.str() + " = vec3(" + setFromValues(inColor->values) + ");\n" + glslVars;
        glslCode = dst.str() + " = vec3(" + colorV.str() + ");\n" + glslCode;  // TODO

        MaterialLink* linkToBright = findLink(links, lastNode, bright, true);
        MaterialLink* linkToContrast = findLink(links, lastNode, contrast, true);
        MaterialLink* linkToColor = findLink(links, lastNode, inColor, true);

        if (linkToBright) processBlenderLink(
            glslCode, glslVars, glslGlobal, links, ss, linkToBright->nodeFrom.get(), linkToBright->pinFrom.get());
        if (linkToContrast) processBlenderLink(
            glslCode, glslVars, glslGlobal, links, ss, linkToContrast->nodeFrom.get(), linkToContrast->pinFrom.get());
        if (linkToColor) processBlenderLink(
            glslCode, glslVars, glslGlobal, links, ss, linkToColor->nodeFrom.get(), linkToColor->pinFrom.get());
    }
    else
        OSG_WARN << "[MaterialGraph] Unsupported link node type: " << lastNode->type << std::endl;
}
