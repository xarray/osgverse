#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "../PropertyInterface.h"
#include "../ImGuiComponents.h"
using namespace osgVerse;

class GeometryPropertyItem : public PropertyItem
{
public:
    GeometryPropertyItem()
    {
        _name = new InputField(ImGuiComponentBase::TR("Name##prop0201"));
        _name->placeholder = ImGuiComponentBase::TR("?? name");
        _name->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };
    }

    virtual std::string title() const { return "Geometry Data"; }
    virtual bool needRefreshUI() const { return false; }

    virtual void updateTarget(ImGuiComponentBase* c)
    {
        if (_type == GeometryType)
        {
            osg::Geometry* g = static_cast<osg::Geometry*>(_target.get());
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

PropertyItem* createGeometryPropertyItem()
{ return new GeometryPropertyItem; }
