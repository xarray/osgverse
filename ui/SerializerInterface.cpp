#include "SerializerInterface.h"
#include "nanoid/nanoid.h"
using namespace osgVerse;

static int g_headerFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Bullet
                         | ImGuiTreeNodeFlags_OpenOnDoubleClick;

SerializerBaseItem::SerializerBaseItem(osg::Object* obj, bool composited)
:   _object(obj), _indent(10.0f), _selected(false), _dirty(true), _hidden(false)
{ _postfix = "##" + nanoid::generate(8); _composited = composited; }

bool SerializerBaseItem::showInternal(ImGuiManager* mgr, ImGuiContentHandler* content,
                                      const std::string& title)
{
    bool toOpen = true; if (_hidden) return false;
    if (ImGui::ArrowButton((title + "_Arrow").c_str(), ImGuiDir_Down))  // TODO: disabled = ImGuiDir_None
    {
        // Select the item and also open popup menu
        ImGui::OpenPopup((title + "_Popup").c_str());
    }
    ImGui::SameLine();

    if (_composited)
    {
        if (_selected) ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.4f, 0.4f, 0.0f, 1.0f));
        else ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
        toOpen = ImGui::CollapsingHeader(title.c_str(), g_headerFlags);
        if (_selected) ImGui::PopStyleColor();
    }

    if (ImGui::BeginPopup((title + "_Popup").c_str()))
    {
        // TODO: up/down/edit/delete for custom interface
        int selection = 0;
        if (ImGui::MenuItem(TR(_selected ? "Unselect" : "Select").c_str())) selection = 1;
        ImGui::EndPopup();

        if (selection) _selected = !_selected;  // FIXME: just for test
    }

    const static float popUpButtonIndent = 30.0f; ImGui::Indent(popUpButtonIndent);
    if (toOpen)
    {
        if (_composited) ImGui::Indent(_indent);
        toOpen = showProperty(mgr, content); _dirty = false;
        if (_composited) ImGui::Unindent(_indent);
    }
    ImGui::Unindent(popUpButtonIndent); return toOpen;
}

std::string SerializerBaseItem::tooltip(const LibraryEntry::Property& prop,
                                    const std::string& postfix) const
{
    std::string body = prop.ownerClass + "\nset" + prop.name + "(" + prop.typeName + ")";
    return body + (postfix.empty() ? "" : ("\n<" + postfix + ">"));
}

SerializerInterface::SerializerInterface(osg::Object* obj, LibraryEntry* entry,
                                         const LibraryEntry::Property& prop, bool composited)
: SerializerBaseItem(obj, composited), _entry(entry), _property(prop) {}

bool SerializerInterface::show(ImGuiManager* mgr, ImGuiContentHandler* content)
{
    std::string title = TR(_property.name) + _postfix;
    return showInternal(mgr, content, title);
}

int SerializerInterface::createSpiderNode(SpiderEditor* spider, bool getter, bool setter)
{
    SpiderEditor::NodeItem* node = spider->createNode(TR(_property.name));
    if (getter) spider->createPin(node, _property.typeName, true);
    if (setter) spider->createPin(node, _property.typeName, false);
    node->owner = _object; return node->id;
}

SerializerFactory* SerializerFactory::instance()
{
    static osg::ref_ptr<SerializerFactory> s_instance = new SerializerFactory;
    return s_instance.get();
}

LibraryEntry* SerializerFactory::createInterfaces(osg::Object* obj, LibraryEntry* lastEntry,
                                                  std::vector<osg::ref_ptr<SerializerBaseItem>>& interfaces)
{
    std::string libName = obj->libraryName(), clsName = obj->className();
    std::string fullName = libName + "::" + clsName; interfaces.clear();
    LibraryEntry* entry = (lastEntry && lastEntry->getLibraryName() == libName)
                        ? lastEntry : new LibraryEntry(libName);
    SerializerFactory* factory = SerializerFactory::instance();
    
    std::vector<LibraryEntry::Property> props = entry->getPropertyNames(clsName);
    std::set<std::string> registeredProps;  // to avoid duplicated props
    for (size_t i = 0; i < props.size(); ++i)
    {
        const std::string propName = props[i].name;
        if (registeredProps.find(propName) != registeredProps.end()) continue;
        if (props[i].outdated) continue; else registeredProps.insert(propName);

        if (_blacklistMap.find(propName) != _blacklistMap.end())
        {
            std::set<std::string>& bl = _blacklistMap[propName];
            if (bl.find("") != bl.end() || bl.find(fullName) != bl.end()) continue;
        }

        osg::ref_ptr<SerializerBaseItem> si;
        if (_userCreatorMap.find(propName) != _userCreatorMap.end())
        {
            InterfaceFunctionMap& funcMap = _userCreatorMap[propName];
            if (funcMap.find(fullName) != funcMap.end()) si = funcMap[fullName](obj, entry, props[i]);
            else if (funcMap.find("") != funcMap.end()) si = funcMap[""](obj, entry, props[i]);
        }

        if (!si) si = factory->createInterface(obj, entry, props[i]);
        if (si.valid()) interfaces.push_back(si);
    }
    return entry;
}

SerializerBaseItem* SerializerFactory::createInterface(osg::Object* obj, LibraryEntry* entry,
                                                       const LibraryEntry::Property& prop)
{
    osgDB::BaseSerializer::Type t = prop.type;
    if (obj == NULL || entry == NULL)
    {
        OSG_WARN << "[SerializerFactory] Empty input arguments for " << prop.name << std::endl;
        return NULL;
    }

    if (_creatorMap.find(t) == _creatorMap.end())
    {
        OSG_WARN << "[SerializerFactory] Interface not implemented for " << prop.ownerClass
                 << "::" << prop.name << std::endl; return NULL;
    }
    return _creatorMap[t](obj, entry, prop);
}
