#include <osg/io_utils>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>

#include "../pipeline/Global.h"
#include "../PropertyInterface.h"
#include "../ImGuiComponents.h"
using namespace osgVerse;

class TexturePropertyItem : public PropertyItem
{
public:
    TexturePropertyItem()
    {
        _addButton = new Button(ImGuiComponentBase::TR("Add Texture##prop0301"));
        // TODO: add texture
    }

    void prepareTextureForGui(const std::string& id, osg::Texture2D* tex)
    { _textureToGui[id] = tex; }

    virtual std::string title() const { return "Texture Data"; }
    virtual bool needRefreshUI() const { return false; }

    virtual void updateTarget(ImGuiComponentBase* c)
    {
        if (_type == StateSetType)
        {
            osg::StateSet* ss = static_cast<osg::StateSet*>(_target.get());
            if (!c)
            {
                osg::StateSet::TextureAttributeList& taList = ss->getTextureAttributeList();
                osg::TexMat* tmat = NULL; _textureMap.clear();
                for (size_t u = 0; u < taList.size(); ++u)
                {
                    osg::StateSet::AttributeList& aList = taList[u];
                    for (osg::StateSet::AttributeList::iterator itr = aList.begin();
                         itr != aList.end(); ++itr)
                    {
                        osg::StateAttribute* sa = itr->second.first.get();
                        if (itr->first.first == osg::StateAttribute::TEXMAT)
                        {
                            tmat = static_cast<osg::TexMat*>(sa);
                            if (_textureMap.find(u) != _textureMap.end())
                            { _textureMap[u].refreshTexMatrix(u, tmat); tmat = NULL; }
                        }
                        else if (itr->first.first == osg::StateAttribute::TEXTURE)
                        {
                            TextureData td; osg::Texture* tex = static_cast<osg::Texture*>(sa);
                            td.createAttributes(this, u, tex, itr->second.second);
                            if (tmat) { _textureMap[u].refreshTexMatrix(u, tmat); tmat = NULL; }
                            _textureMap[u] = td;
                        }
                    }
                }
            }
            else
            {
                // TODO
            }
        }
    }

    virtual bool show(ImGuiManager* mgr, ImGuiContentHandler* content)
    {
        if (!_textureToGui.empty())
        {
            for (std::map<std::string, osg::observer_ptr<osg::Texture2D>>::iterator
                 itr = _textureToGui.begin(); itr != _textureToGui.end(); ++itr)
            {
                osg::Texture2D* tex = itr->second.get(); if (!tex) continue;
                mgr->setGuiTexture(itr->first, tex);
            }
            _textureToGui.clear();
        }

        bool updated = false;
        for (std::map<int, TextureData>::iterator itr = _textureMap.begin();
             itr != _textureMap.end(); ++itr)
        {
            std::string title = ImGuiComponentBase::TR("Texture Unit ")
                              + std::to_string(itr->first) + "##prop0300";
            if (ImGui::TreeNode((uniformNames[itr->first] + " / " + title).c_str()))
            {
                TextureData& td = itr->second;
                td._image->show(mgr, content);
                td._description->show(mgr, content);
                td._onOff->show(mgr, content); ImGui::SameLine();
                td._overrided->show(mgr, content); ImGui::SameLine();
                td._protected->show(mgr, content);
                td._useNPOT->show(mgr, content);
                td._wrap->show(mgr, content);
                td._filter->show(mgr, content);
                td._uvOffset->show(mgr, content);
                td._uvTiling->show(mgr, content);
                td._removeButton->show(mgr, content);
                ImGui::TreePop();
            }
        }

        ImGui::Separator();
        updated |= _addButton->show(mgr, content);
        return updated;
    }

protected:
    struct TextureData
    {
        osg::ref_ptr<ImageButton> _image;
        osg::ref_ptr<CheckBox> _onOff, _overrided, _protected;
        osg::ref_ptr<CheckBox> _useNPOT;
        osg::ref_ptr<ComboBox> _wrap, _filter;
        osg::ref_ptr<Label> _description;
        osg::ref_ptr<InputVectorField> _uvOffset, _uvTiling;
        osg::ref_ptr<Button> _removeButton;

        void createAttributes(TexturePropertyItem* tp, int u, osg::Texture* tex, unsigned int value)
        {
            std::string id = "##propU" + std::to_string(u);
            generateImageButton(tp, tex, u);

            _onOff = new CheckBox(ImGuiComponentBase::TR("On") + id + "0303",
                                  value & osg::StateAttribute::ON);
            _onOff->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _overrided = new CheckBox(ImGuiComponentBase::TR("Override") + id + "0303",
                                      value & osg::StateAttribute::OVERRIDE);
            _overrided->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _protected = new CheckBox(ImGuiComponentBase::TR("Protected") + id + "0303",
                                      value & osg::StateAttribute::PROTECTED);
            _protected->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _useNPOT = new CheckBox(ImGuiComponentBase::TR("Use Non-Power-Of-Two Size") + id + "0304",
                                    !tex->getResizeNonPowerOfTwoHint());
            _useNPOT->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _wrap = new ComboBox(ImGuiComponentBase::TR("Edge Wrap") + id + "0305");
            _wrap->items.push_back(ImGuiComponentBase::TR("Clamped"));
            _wrap->items.push_back(ImGuiComponentBase::TR("Edge Color"));
            _wrap->items.push_back(ImGuiComponentBase::TR("Border Color"));
            _wrap->items.push_back(ImGuiComponentBase::TR("Repeated"));
            _wrap->items.push_back(ImGuiComponentBase::TR("Mirrored"));
            switch (tex->getWrap(osg::Texture::WRAP_S))
            {
            case osg::Texture::CLAMP_TO_EDGE: _wrap->index = 1; break;
            case osg::Texture::CLAMP_TO_BORDER: _wrap->index = 2; break;
            case osg::Texture::REPEAT: _wrap->index = 3; break;
            case osg::Texture::MIRROR: _wrap->index = 4; break;
            }
            _wrap->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _filter = new ComboBox(ImGuiComponentBase::TR("Filter") + id + "0306");
            _filter->items.push_back(ImGuiComponentBase::TR("Linear"));
            _filter->items.push_back(ImGuiComponentBase::TR("Mipmap Trilinear"));
            _filter->items.push_back(ImGuiComponentBase::TR("Mipmap Bilinear"));
            _filter->items.push_back(ImGuiComponentBase::TR("Mipmap Linear"));
            _filter->items.push_back(ImGuiComponentBase::TR("Mipmap Point"));
            _filter->items.push_back(ImGuiComponentBase::TR("Nearest"));
            switch (tex->getFilter(osg::Texture::MIN_FILTER))
            {
            case osg::Texture::LINEAR_MIPMAP_LINEAR: _filter->index = 1; break;
            case osg::Texture::LINEAR_MIPMAP_NEAREST: _filter->index = 2; break;
            case osg::Texture::NEAREST_MIPMAP_LINEAR: _filter->index = 3; break;
            case osg::Texture::NEAREST_MIPMAP_NEAREST: _filter->index = 4; break;
            case osg::Texture::NEAREST: _filter->index = 5; break;
            }
            _filter->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _uvOffset = new InputVectorField(ImGuiComponentBase::TR("UV Offset") + id + "0307");
            _uvOffset->vecValue.set(0.0f, 0.0f, 1.0f, 1.0f); _uvOffset->vecNumber = 2;
            _uvOffset->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _uvTiling = new InputVectorField(ImGuiComponentBase::TR("UV Tiling") + id + "0308");
            _uvTiling->vecValue.set(1.0f, 1.0f, 1.0f, 1.0f); _uvTiling->vecNumber = 2;
            _uvTiling->callback = [tp](ImGuiManager*, ImGuiContentHandler*, ImGuiComponentBase* me)
            { tp->updateTarget(me); };

            _description = new Label;
            _description->texts.push_back(getTextureName(tex));
            _description->texts.push_back(getTextureResolutionString(tex));

            _removeButton = new Button(ImGuiComponentBase::TR("Remove Texture") + id + "0309");
            // TODO: remove texture
        }

        void refreshTexMatrix(int u, osg::TexMat* tmat)
        {
            // TODO: _uvOffset, _uvTiling
        }

        void generateImageButton(TexturePropertyItem* tp, osg::Texture* tex, int u)
        {
            // FIXME: tex1D/2D/3d/array/cube
            std::string id = "TexturePreview" + std::to_string(u);
            _image = new ImageButton(id); _image->size.set(256, 256);

            osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(tex);
            if (tex2D) tp->prepareTextureForGui(id, tex2D);
        }

        std::string getTextureName(osg::Texture* tex)
        {
            std::stringstream ss; ss << tex->className();
            osg::Texture2D* tex2D = dynamic_cast<osg::Texture2D*>(tex);
            if (tex2D && tex2D->getImage()) ss << ": " << tex2D->getImage()->getFileName();
            return ss.str();
        }

        std::string getTextureResolutionString(osg::Texture* tex)
        {
            std::stringstream ss;
            ss << tex->getTextureWidth() << " x " << tex->getTextureHeight();
            if (tex->getTextureDepth() > 1) ss << " x " << tex->getTextureDepth();

            switch (tex->getInternalFormat())
            {
            case GL_RGB: case GL_RGB8: ss << " RGB8"; break;
            case GL_RGBA: case GL_RGBA8: ss << " RGBA8"; break;
            case GL_LUMINANCE: case GL_R8: ss << " R8"; break;
            case GL_RG8: ss << " RG8"; break;
            case GL_RGB16F_ARB: ss << " RGB16F"; break;
            case GL_RGB32F_ARB: ss << " RGB32F"; break;
            case GL_RGBA16F_ARB: ss << " RGBA16F"; break;
            case GL_RGBA32F_ARB: ss << " RGBA32F"; break;
            case GL_COMPRESSED_RGB_S3TC_DXT1_EXT: ss << " DXT1"; break;
            case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT: ss << " DXT1a"; break;
            case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT: ss << " DXT5"; break;
            default: ss << " Type-" << tex->getInternalFormat(); break;
            }
            return ss.str();
        }
    };
    std::map<int, TextureData> _textureMap;
    std::map<std::string, osg::observer_ptr<osg::Texture2D>> _textureToGui;
    osg::ref_ptr<Button> _addButton;
};

PropertyItem* createTexturePropertyItem()
{ return new TexturePropertyItem; }
