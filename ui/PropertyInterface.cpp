#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "PropertyInterface.h"
#include "ImGuiComponents.h"
using namespace osgVerse;

class BasicPropertyItem : public PropertyItem
{
public:
    virtual std::string title() const { return (_type == NodeType) ? "Node Basics" : "Drawable Basics"; }
    virtual bool needRefreshUI() const { return true; }

    BasicPropertyItem()
    {
        _name = new InputField(ImGuiComponentBase::TR("Name"));
        _name->placeholder = ImGuiComponentBase::TR(
            (_type == NodeType) ? "Node name" : "Drawable name");
        _name->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };

        _mask = new InputValueField(ImGuiComponentBase::TR("Mask"));
        _mask->type = InputValueField::UIntValue;
        _mask->flags = ImGuiInputTextFlags_CharsHexadecimal;
        _mask->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };
    }

    virtual void updateTarget(ImGuiComponentBase* c)
    {
        if (_type == NodeType)
        {
            osg::Node* n = static_cast<osg::Node*>(_target.get());
            if (!c)
            {
                _name->value = n->getName();
                _mask->value = n->getNodeMask();
            }
            else
            {
                if (c == _name) n->setName(((InputField*)c)->value);
                else if (c == _mask) n->setNodeMask(((InputValueField*)c)->value);
            }
        }
        else if (_type == DrawableType)
        {
            osg::Drawable* d = static_cast<osg::Drawable*>(_target.get());
            if (!c) _name->value = d->getName();
            else if (c == _name) d->setName(((InputField*)c)->value);
        }
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        bool updated = _name->show(mgr, content);
        if (_type == NodeType) updated |= _mask->show(mgr, content);
        return updated;
    }

protected:
    osg::ref_ptr<InputField> _name;
    osg::ref_ptr<InputValueField> _mask;
};

///////////////////////////

PropertyItemManager* PropertyItemManager::instance()
{
    static osg::ref_ptr<PropertyItemManager> s_instance = new PropertyItemManager;
    return s_instance.get();
}

PropertyItemManager::PropertyItemManager()
{
    _standardItemMap[BasicNodeItem] = new BasicPropertyItem;
    _standardItemMap[BasicDrawableItem] = new BasicPropertyItem;
}

PropertyItem* PropertyItemManager::getStandardItem(StandardItemType t)
{
    if (_standardItemMap.find(t) == _standardItemMap.end()) return NULL;
    else return _standardItemMap[t];
}

PropertyItem* PropertyItemManager::getExtendedItem(const std::string& t)
{
    if (_extendedItemMap.find(t) == _extendedItemMap.end()) return NULL;
    else return _extendedItemMap[t];
}
