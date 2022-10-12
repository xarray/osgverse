#include <osg/io_utils>
#include <osg/Version>
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
        _attributes = new ListView(ImGuiComponentBase::TR("Attributes##prop0201"));
        _primitives = new ListView(ImGuiComponentBase::TR("Primitives##prop0202"));

        _useVBO = new CheckBox(ImGuiComponentBase::TR("Use Vertex Buffer Objects##prop0203"), false);
        _useVBO->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };

        _useDisplayLists = new CheckBox(ImGuiComponentBase::TR("Use Display Lists##prop0204"), false);
        _useDisplayLists->callback = [&](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
        { updateTarget(me); };

        _modifyButton = new Button(ImGuiComponentBase::TR("Modify Geometry##prop0205"));
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
                std::vector<osg::ref_ptr<ListView::ListData>>& it = _attributes->items; it.clear();
                addListItemFromArray(it, ImGuiComponentBase::TR("Vertices"), g->getVertexArray());
                addListItemFromArray(it, ImGuiComponentBase::TR("Normal"), g->getNormalArray());
                addListItemFromArray(it, ImGuiComponentBase::TR("Color"), g->getColorArray());
                addListItemFromArray(it, ImGuiComponentBase::TR("Tangent"), g->getVertexAttribArray(6));
                addListItemFromArray(it, ImGuiComponentBase::TR("Binormal"), g->getVertexAttribArray(7));
                addListItemFromArray(it, ImGuiComponentBase::TR("Weights"), g->getVertexAttribArray(1));
                addListItemFromArray(it, ImGuiComponentBase::TR("UV0"), g->getTexCoordArray(0));
                addListItemFromArray(it, ImGuiComponentBase::TR("UV1"), g->getTexCoordArray(1));
                addListItemFromArray(it, ImGuiComponentBase::TR("UV2"), g->getTexCoordArray(2));

                std::vector<osg::ref_ptr<ListView::ListData>>& it2 = _primitives->items; it2.clear();
                for (unsigned int i = 0; i < g->getNumPrimitiveSets(); ++i)
                    addListItemFromPrimitives(it2, i, g->getPrimitiveSet(i));

                _useVBO->value = g->getUseVertexBufferObjects();
                _useDisplayLists->value = g->getUseDisplayList();
            }
            else
            {
                if (c == _useVBO) g->setUseVertexBufferObjects(_useVBO->value);  // TODO: set basic-info command
                if (c == _useDisplayLists) g->setUseDisplayList(_useDisplayLists->value);
            }
        }
    }

    void addListItemFromArray(std::vector<osg::ref_ptr<ListView::ListData>>& items,
                              const std::string& prefix, osg::Array* arr)
    {
        std::string type = "Unknown";
        if (!arr) return; std::stringstream ss;

        switch (arr->getDataType())
        {
        case GL_UNSIGNED_BYTE: type = "UByte"; break;
        case GL_BYTE: type = "Byte"; break;
        case GL_UNSIGNED_SHORT: type = "UShort"; break;
        case GL_SHORT: type = "Short"; break;
        case GL_UNSIGNED_INT: type = "UInt"; break;
        case GL_INT: type = "Int"; break;
        case GL_FLOAT: type = "Float"; break;
        case GL_DOUBLE: type = "Double"; break;
        }
        ss << prefix << ": " << type << arr->getDataSize() << ", " << arr->getNumElements();

        osg::ref_ptr<ListView::ListData> lData = new ListView::ListData;
        lData->name = ss.str(); items.push_back(lData);
    }

    void addListItemFromPrimitives(std::vector<osg::ref_ptr<ListView::ListData>>& items,
                                   unsigned int idx, osg::PrimitiveSet* p)
    {
        std::string type = "Unknown", mode = "Unknown", id = std::to_string(idx);
        if (!p) return; osg::DrawElements* de = p->getDrawElements();
        
        switch (p->getMode())
        {
        case GL_POINTS: mode = "Points" + id; break;
        case GL_LINES: mode = "Lines" + id; break;
        case GL_LINE_STRIP: mode = "LineStrip" + id; break;
        case GL_TRIANGLES: mode = "Triangles" + id; break;
        case GL_TRIANGLE_STRIP: mode = "TriStrip" + id; break;
        case GL_TRIANGLE_FAN: mode = "TriFan" + id; break;
        case GL_QUADS: mode = "Quads" + id; break;
        case GL_QUAD_STRIP: mode = "QuadStrip" + id; break;
        }

        std::stringstream ss;
        if (de != NULL)
        {
            switch (de->getType())
            {
            case osg::PrimitiveSet::DrawElementsUBytePrimitiveType: type = "UByte"; break;
            case osg::PrimitiveSet::DrawElementsUShortPrimitiveType: type = "UShort"; break;
            case osg::PrimitiveSet::DrawElementsUIntPrimitiveType: type = "UInt"; break;
            }
            ss << mode << ": " << type << " x " << de->getNumIndices();
        }
        else
        {
            osg::DrawArrays* da = dynamic_cast<osg::DrawArrays*>(p);
            if (da)
                ss << mode << ": [" << da->getFirst() << ", " << (da->getFirst() + da->getCount()) << ")";
            else
                ss << mode << " (" << p->className() << ")";
        }

        osg::ref_ptr<ListView::ListData> lData = new ListView::ListData;
        lData->name = ss.str(); items.push_back(lData);
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        bool updated = _attributes->show(mgr, content);
        updated |= _primitives->show(mgr, content);
        updated |= _modifyButton->show(mgr, content);
        updated |= _useVBO->show(mgr, content);
        updated |= _useDisplayLists->show(mgr, content);
        return updated;
    }

protected:
    osg::ref_ptr<ListView> _attributes, _primitives;
    osg::ref_ptr<CheckBox> _useVBO, _useDisplayLists;
    osg::ref_ptr<Button> _modifyButton;
};

PropertyItem* createGeometryPropertyItem()
{ return new GeometryPropertyItem; }
