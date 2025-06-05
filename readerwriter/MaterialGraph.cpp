#include <picojson.h>
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

MaterialGraph* MaterialGraph::instance()
{
    static osg::ref_ptr<MaterialGraph> s_instance = new MaterialGraph;
    return s_instance.get();
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

    if (dataRoot.contains("VERSE_material_graph"))
    {
        std::string graphString = dataRoot.get("VERSE_material_graph").get<std::string>();
        err = picojson::parse(graphRoot, graphString);
        if (!err.empty())
        {
            OSG_WARN << "[MaterialGraph] Failed to load 'VERSE_material_graph' from Blender graph: "
                     << err << std::endl; return false;
        }

        std::map<std::string, osg::ref_ptr<MaterialNode>> nodes;
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
                    node->name = name; node->type = type;
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

                            MaterialPin* pin = new MaterialPin; node->inputs[name] = pin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
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

                            MaterialPin* pin = new MaterialPin; node->outputs[name] = pin;
                            pin->name = name; pin->type = type; pin->values = readFromValues(def);
                        }
                    }
                    nodes[name] = node;
                }
            }
        }

        std::vector<osg::ref_ptr<MaterialLink>> links;
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

        std::cout << graphRoot.serialize(true) << std::endl;  // TODO
        return true;
    }
    return false;
}
