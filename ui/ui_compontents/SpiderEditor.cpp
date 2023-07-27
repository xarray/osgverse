#include "../ImGuiComponents.h"
#include <imgui/imgui_internal.h>
#include <imgui/imgui-knobs.h>
#include <imgui/ImGuiFileDialog.h>
#include <imgui/node-editor/imgui_node_editor.h>

namespace ed = ax::NodeEditor;
using namespace osgVerse;
int g_counter = 1;

/// SpiderEditor related

int SpiderEditor::NodeItem::findPin(const std::string& n, bool isOut) const
{
    if (isOut)
    {
        for (std::map<int, osg::ref_ptr<PinItem>>::const_iterator itr = outPins.begin();
             itr != outPins.end(); ++itr) { if (itr->second->name == n) return itr->first; }
    }
    else
    {
        for (std::map<int, osg::ref_ptr<PinItem>>::const_iterator itr = inPins.begin();
             itr != inPins.end(); ++itr) { if (itr->second->name == n) return itr->first; }
    }
    return 0;
}

SpiderEditor::NodeItem* SpiderEditor::createNode(const std::string& n)
{
    osg::ref_ptr<NodeItem> item = new NodeItem;
    item->name = n + "##spider" + std::to_string(g_counter);
    nodes[g_counter++] = item; item->id = g_counter; return item.get();
}

SpiderEditor::PinItem* SpiderEditor::createPin(NodeItem* it, const std::string& n, bool isOut)
{
    osg::ref_ptr<PinItem> pin = new PinItem;
    if (isOut) it->outPins[g_counter] = pin;  else it->inPins[g_counter] = pin;
    pin->name = n + "##spider" + std::to_string(g_counter++);
    pin->nodeId = it->id; return pin.get();
}

SpiderEditor::LinkItem* SpiderEditor::createLink(NodeItem* src, const std::string& srcPin,
                                                 NodeItem* dst, const std::string& dstPin)
{
    osg::ref_ptr<LinkItem> link = new LinkItem;
    link->inNode = src; link->inPin = src->findPin(srcPin, false);
    link->outNode = dst; link->outPin = src->findPin(dstPin, true);
    links[g_counter++] = link; return link.get();
}

void SpiderEditor::createEditor(const std::string& cfg)
{
    ed::Config configData;
    configData.SettingsFile = cfg.c_str();
    editorContext = ed::CreateEditor(&configData);
}

void SpiderEditor::destroyEditor()
{
    ed::DestroyEditor((ed::EditorContext*)editorContext);
    editorContext = NULL;
}

bool SpiderEditor::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    if (!editorContext) return false;
    ed::SetCurrentEditor((ed::EditorContext*)editorContext);

    ed::Begin(name.c_str(), ImVec2(size[0], size[1]));
    for (std::map<int, osg::ref_ptr<NodeItem>>::iterator itr = nodes.begin();
         itr != nodes.end(); ++itr)
    {
        NodeItem* nodeItem = itr->second.get();
        ed::BeginNode(itr->first);
        if (nodeItem != NULL)
        {
            ImGui::Text("%s", nodeItem->name.c_str());
            ImGui::Separator();

            std::vector<int> inIds, outIds;
            for (std::map<int, osg::ref_ptr<PinItem>>::iterator itr2 = nodeItem->inPins.begin();
                 itr2 != nodeItem->inPins.end(); ++itr2) inIds.push_back(itr2->first);
            for (std::map<int, osg::ref_ptr<PinItem>>::iterator itr2 = nodeItem->outPins.begin();
                itr2 != nodeItem->outPins.end(); ++itr2) outIds.push_back(itr2->first);

            size_t num = osg::minimum(inIds.size(), outIds.size());
            for (size_t i = 0; i < num; ++i)
            {
                ed::BeginPin(inIds[i], ed::PinKind::Input);
                    showPin(mgr, content, nodeItem->inPins[inIds[i]].get());
                ed::EndPin(); ImGui::SameLine();

                ed::BeginPin(outIds[i], ed::PinKind::Output);
                    showPin(mgr, content, nodeItem->outPins[outIds[i]].get());
                ed::EndPin();
            }

            for (size_t i = num; i < inIds.size(); ++i)
            {
                ed::BeginPin(inIds[i], ed::PinKind::Input);
                    showPin(mgr, content, nodeItem->inPins[inIds[i]].get());
                ed::EndPin(); ImGui::SameLine();
            }

            for (size_t i = num; i < outIds.size(); ++i)
            {
                ed::BeginPin(inIds[i], ed::PinKind::Output);
                    showPin(mgr, content, nodeItem->outPins[outIds[i]].get());
                ed::EndPin(); ImGui::SameLine();
            }
        }
        ed::EndNode();
    }

    for (std::map<int, osg::ref_ptr<LinkItem>>::iterator itr = links.begin();
         itr != links.end(); ++itr)
    {
        LinkItem* link = itr->second.get();
        ed::Link(itr->first, link->inPin, link->outPin,
                 ImVec4(link->color[0], link->color[1], link->color[2], link->color[3]));
    }
    ed::End();
    ed::SetCurrentEditor(NULL);
    return false;
}

void SpiderEditor::showPin(ImGuiManager* mgr, ImGuiContentHandler* content, PinItem* pin)
{
    ImGui::Text("%s", pin->name.c_str());
}
