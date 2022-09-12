#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "../PropertyInterface.h"
#include "../ImGuiComponents.h"
using namespace osgVerse;

class TexturePropertyItem : public PropertyItem
{
public:
    TexturePropertyItem()
    {
        _name = new InputField(ImGuiComponentBase::TR("Name##prop0301"));
        _name->placeholder = ImGuiComponentBase::TR("?? name");
        _name->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };
    }

    virtual std::string title() const { return "Texture Data"; }
    virtual bool needRefreshUI() const { return false; }

    virtual void updateTarget(ImGuiComponentBase* c)
    {
        if (_type == StateSetType)
        {
            osg::StateSet* ss = static_cast<osg::StateSet*>(_target.get());
            if (!c)
            {
            }
            else
            {
            }
        }
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        bool updated = _name->show(mgr, content);
        return updated;
    }

protected:
    osg::ref_ptr<InputField> _name;
};

PropertyItem* createTexturePropertyItem()
{ return new TexturePropertyItem; }
